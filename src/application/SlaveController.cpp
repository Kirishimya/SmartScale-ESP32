#include <application/SlaveController.h>

#include <application/slave/State.h>
#include <application/slave/StateFactory.h>
#include <models/Payloads.h>
#include <services/Logger.h>

#include <cmath>
#include <cstring>
#include <iostream>
#include <chrono>

#ifdef ARDUINO
#include <Arduino.h>
#include <Preferences.h>
#include <Update.h>
#include <esp_system.h>

namespace {
    Preferences preferences;    
}   
#endif



using namespace std::chrono;

namespace {

#ifndef ARDUINO
uint64_t hostMillis()
{
    return duration_cast<milliseconds>(
               steady_clock::now().time_since_epoch())
        .count();
}
#endif

int32_t kgToMilligrams(float kg)
{
    return static_cast<int32_t>(std::lround(kg * 1000000.0f));
}

uint32_t unitsToMilli(float units)
{
    if (units <= 0.0f)
        return 0;
    return static_cast<uint32_t>(std::lround(units * 1000.0f));
}

uint32_t updateFnv1a(uint32_t checksum, const std::vector<uint8_t> &data)
{
    for (uint8_t byte : data) {
        checksum ^= byte;
        checksum *= 16777619UL;
    }
    return checksum;
}

uint64_t applyOffset(uint64_t monotonicMs, int64_t offsetMs)
{
    const int64_t adjusted = static_cast<int64_t>(monotonicMs) + offsetMs;
    return adjusted > 0 ? static_cast<uint64_t>(adjusted) : monotonicMs;
}

} // namespace

//==============================================================
// Constructor
//==============================================================

SlaveController::SlaveController(IEspNowDriver &driver)
    : _network(driver, 0),
      _sm(*this),
      _outQueue(256),
      _lastHeartbeat(0),
      _lastSequence(0),
      _heartbeatInterval(5000),
      _nodeId(0),
      _lastUpdate(0),
      _masterId(1)
{
}

//==============================================================
// Initialization
//==============================================================

bool SlaveController::begin()
{
    if (!_network.begin())
        return false;

    _network.setPacketHandler(
        [this](const Packet &packet, const NetworkManager::MacAddress &mac) {
            handlePacket(packet, mac);
        });
    _network.setDeliveryHandler(
        [this](const NetworkManager::MacAddress &, bool success) {
            if (!success)
                log("ESP-NOW delivery failed.");
        });

    if (!_outQueue.begin("slave_tx"))
        return false;

    if (!_sensors.begin())
        return false;
    
    #ifdef ARDUINO
    preferences.begin("slave", false);
    _nodeId = preferences.getUShort("node_id", 0);
    _masterId = preferences.getUShort("master_id", _masterId);
    _heartbeatInterval = preferences.getUInt("heartbeat_ms", _heartbeatInterval);
    #endif
    _network.setLocalNodeId(_nodeId);
    
    auto boot = makeState("BOOT", _sm, *this);

    if (!boot)
        return false;

    _sm.transitionTo(std::move(boot));

    log("SlaveController initialized.");

    return true;
}

//==============================================================
// Main Loop
//==============================================================

void SlaveController::loop()
{
    _network.poll();

    uint64_t now = nowMillis();

    uint32_t dt = 0;

    if (_lastUpdate != 0)
        dt = static_cast<uint32_t>(now - _lastUpdate);

    _lastUpdate = now;

    //----------------------------------------------------------
    // Update State Machine
    //----------------------------------------------------------

    _sm.update(dt);

    //----------------------------------------------------------
    // Heartbeat
    //----------------------------------------------------------

    if (_sm.current() != nullptr &&
        std::strcmp(_sm.current()->name(), "READY") == 0)
    {
        if (now - _lastHeartbeat >= _heartbeatInterval)
        {
            Packet hb;

            hb.type = PacketType::HEARTBEAT;
            sendPacket(hb);

            _lastHeartbeat = now;
        }
    }

    //----------------------------------------------------------
    // Flush transmission queue
    //----------------------------------------------------------

    QueuedPacket queued;

    if (_outQueue.popReady(queued,
                           static_cast<uint32_t>(now),
                           1000))
    {
        if (!_network.send({queued.bytes, queued.destination, queued.sequence})) {
            log("Failed to transmit queued sensor packet.");
        }
    }
}

//==============================================================
// Time
//==============================================================

uint64_t SlaveController::nowMillis()
{
#ifdef ARDUINO
    return ::millis();
#else
    return hostMillis();
#endif
}

//==============================================================
// Packet Transmission
//==============================================================

void SlaveController::sendPacket(const Packet &packet)
{
    Packet outgoing = packet;
    auto destination = destinationFor(outgoing);
    if (!destination.has_value()) {
        log("Packet dropped: master is not known yet.");
        return;
    }
    if (outgoing.dst_id == 0 && _masterMac.has_value()) {
        outgoing.dst_id = _masterId;
    }

    auto encoded = _network.encode(outgoing, *destination);
    if (!encoded.has_value()) {
        log("Packet dropped: invalid protocol envelope.");
        return;
    }

    if (outgoing.type == PacketType::SENSOR_DATA) {
        if (!_outQueue.push(encoded->bytes, encoded->destination, encoded->sequence)) {
            log("Sensor queue is full; sample was discarded.");
        }
        return;
    }

    if (!_network.send(*encoded)) {
        log("Packet transmission failed.");
    }
}

//==============================================================

void SlaveController::broadcastDiscovery()
{
    Packet pkt;

    pkt.type = PacketType::JOIN_REQUEST;
    pkt.payload = {0x01}; // protocol capability: scale sensor data
    if (!_network.broadcast(pkt)) {
        log("Discovery broadcast failed.");
    }
}

void SlaveController::sendSensorSample()
{
    const float weight = _sensors.readWeight();
    sendSensorData(weight, 0.0f, 0.0f, 0, nowMillis());
}

//==============================================================

void SlaveController::persistNodeId(uint16_t nodeId)
{
    _nodeId = nodeId;
    _network.setLocalNodeId(nodeId);

#ifdef ARDUINO
    preferences.putUShort("node_id", nodeId);
#endif

    log("Node ID persisted: " + std::to_string(nodeId));
}

//==============================================================

void SlaveController::log(const std::string &msg)
{
    Logger::instance().info("slave", msg);
}

//==============================================================

void SlaveController::requestStateTransition(
    const std::string &stateName)
{
    auto state = makeState(
        stateName,
        _sm,
        *this);

    if (state)
        _sm.transitionTo(std::move(state));
}

//==============================================================
// Sensor Packet
//==============================================================

void SlaveController::sendSensorData(
    float weight,
    float weightPerUnit,
    float estimatedUnits,
    uint32_t seq,
    uint64_t ts)
{
    Packet pkt;

    pkt.type = PacketType::SENSOR_DATA;
    pkt.src_id = _nodeId;
    pkt.seq = seq;
    pkt.ts = ts;
    pkt.payload = Payloads::encodeSensorData({
        kgToMilligrams(weight),
        kgToMilligrams(weightPerUnit),
        unitsToMilli(estimatedUnits),
        applyOffset(ts, _timeOffsetMs)
    });

    sendPacket(pkt);
}

//==============================================================
// Receive Handler
//==============================================================

std::optional<NetworkManager::MacAddress> SlaveController::destinationFor(const Packet &packet) const
{
    if (packet.type == PacketType::JOIN_REQUEST || packet.type == PacketType::DISCOVERY) {
        return NetworkManager::broadcastMac();
    }
    return _masterMac;
}

void SlaveController::handlePacket(
    const Packet &packet,
    const NetworkManager::MacAddress &mac)
{

    //----------------------------------------------------------
    // Packet Dispatch
    //----------------------------------------------------------

    switch (packet.type)
    {
        case PacketType::JOIN_ACCEPT:
        {
            _masterMac = mac;
            _masterId = packet.src_id;
            persistNodeId(packet.dst_id);

            log("JOIN_ACCEPT received.");

            requestStateTransition("TIME_SYNC");

            break;
        }

        case PacketType::TIME_SYNC:
        {
            Payloads::TimeSync sync;
            if (Payloads::decodeTimeSync(packet.payload, sync)) {
                _timeOffsetMs = static_cast<int64_t>(sync.epochMs) - static_cast<int64_t>(nowMillis());
                log("TIME_SYNC received. Offset=" + std::to_string(_timeOffsetMs) + " ms.");
            } else {
                log("TIME_SYNC received without valid payload.");
            }

            requestStateTransition("READY");

            break;
        }

        case PacketType::ACK:
        {
            uint32_t acknowledged = 0;
            if (Payloads::decodeAck(packet.payload, acknowledged)) {
                if (!_outQueue.markAcked(acknowledged)) {
                    log("ACK received for unknown queued packet.");
                }
                requestStateTransition("READY");
            }

            break;
        }

        case PacketType::NACK:
        {
            requestStateTransition("RECOVERY");

            break;
        }

        case PacketType::PING:
        {
            Packet pong;

            pong.type = PacketType::PONG;
            pong.src_id = _nodeId;
            pong.dst_id = packet.src_id;
            sendPacket(pong);
            requestStateTransition("SEND_DATA");

            break;
        }

        case PacketType::COMMAND:
        {
            handleCommand(packet);

            break;
        }

        case PacketType::CONFIG:
        {
            handleConfig(packet);

            break;
        }

        case PacketType::OTA_BEGIN:
        case PacketType::OTA_DATA:
        case PacketType::OTA_END:
        {
            handleOta(packet);

            break;
        }

        default:
            break;
    }
}

void SlaveController::sendAck(uint32_t sequence)
{
    Packet ack;
    ack.type = PacketType::ACK;
    ack.dst_id = _masterId;
    ack.payload = Payloads::encodeAck(sequence);
    sendPacket(ack);
}

void SlaveController::sendNack(uint32_t sequence, const std::string &reason)
{
    Packet nack;
    nack.type = PacketType::NACK;
    nack.dst_id = _masterId;
    nack.payload = Payloads::encodeAck(sequence);
    const auto text = Payloads::encodeText(reason);
    nack.payload.insert(nack.payload.end(), text.begin(), text.end());
    sendPacket(nack);
}

void SlaveController::handleCommand(const Packet &packet)
{
    if (packet.payload.empty()) {
        sendNack(packet.seq, "empty command");
        return;
    }

    const auto command = static_cast<Payloads::CommandId>(packet.payload[0]);
    switch (command) {
        case Payloads::CommandId::SendDiagnostic:
        {
            Packet diagnostic;
            diagnostic.type = PacketType::DIAGNOSTIC;
            diagnostic.dst_id = _masterId;
            diagnostic.payload = Payloads::encodeText(
                "uptime_ms=" + std::to_string(nowMillis()) +
                ";queue=" + std::to_string(_outQueue.size()));
            sendPacket(diagnostic);
            sendAck(packet.seq);
            break;
        }
        case Payloads::CommandId::Rediscover:
            _masterMac.reset();
            sendAck(packet.seq);
            requestStateTransition("DISCOVERY");
            break;
        case Payloads::CommandId::Reboot:
            sendAck(packet.seq);
#ifdef ARDUINO
            esp_restart();
#endif
            break;
        case Payloads::CommandId::Tare:
            sendNack(packet.seq, "tare command is not wired to ScaleManager yet");
            break;
        default:
            sendNack(packet.seq, "unknown command");
            break;
    }
}

void SlaveController::handleConfig(const Packet &packet)
{
    if (packet.payload.size() < 5) {
        sendNack(packet.seq, "invalid config payload");
        return;
    }

    const auto config = static_cast<Payloads::ConfigId>(packet.payload[0]);
    uint32_t value = 0;
    if (!Payloads::readU32(packet.payload, 1, value)) {
        sendNack(packet.seq, "invalid config value");
        return;
    }

    switch (config) {
        case Payloads::ConfigId::HeartbeatIntervalMs:
            if (value < 1000 || value > 3600000UL) {
                sendNack(packet.seq, "heartbeat interval out of range");
                return;
            }
            _heartbeatInterval = value;
#ifdef ARDUINO
            preferences.putUInt("heartbeat_ms", _heartbeatInterval);
#endif
            sendAck(packet.seq);
            break;
        case Payloads::ConfigId::MasterNodeId:
            if (value == 0 || value > UINT16_MAX) {
                sendNack(packet.seq, "master id out of range");
                return;
            }
            _masterId = static_cast<uint16_t>(value);
#ifdef ARDUINO
            preferences.putUShort("master_id", _masterId);
#endif
            sendAck(packet.seq);
            break;
        default:
            sendNack(packet.seq, "unknown config");
            break;
    }
}

void SlaveController::handleOta(const Packet &packet)
{
    if (packet.payload.empty() || packet.payload[0] != 1) {
        sendNack(packet.seq, "invalid ota schema");
        return;
    }

    if (packet.type == PacketType::OTA_BEGIN) {
        uint32_t size = 0;
        uint32_t checksum = 0;
        if (!Payloads::readU32(packet.payload, 1, size) ||
            !Payloads::readU32(packet.payload, 5, checksum) ||
            size == 0) {
            sendNack(packet.seq, "invalid ota begin");
            return;
        }
        _ota.active = true;
        _ota.installReady = beginOtaInstall(size);
        if (!_ota.installReady) {
            _ota.active = false;
            sendNack(packet.seq, "ota install partition unavailable");
            return;
        }
        _ota.expectedSize = size;
        _ota.expectedChecksum = checksum;
        _ota.received = 0;
        _ota.checksum = 2166136261UL;
        sendAck(packet.seq);
        return;
    }

    if (!_ota.active) {
        sendNack(packet.seq, "ota session not active");
        return;
    }

    if (packet.type == PacketType::OTA_DATA) {
        uint32_t offset = 0;
        if (!Payloads::readU32(packet.payload, 1, offset) || offset != _ota.received) {
            sendNack(packet.seq, "ota offset mismatch");
            return;
        }
        std::vector<uint8_t> chunk(packet.payload.begin() + 5, packet.payload.end());
        if (_ota.received + chunk.size() > _ota.expectedSize) {
            abortOtaInstall();
            sendNack(packet.seq, "ota data exceeds expected size");
            return;
        }
        if (!writeOtaChunk(chunk)) {
            _ota.active = false;
            sendNack(packet.seq, "ota write failed");
            return;
        }
        _ota.checksum = updateFnv1a(_ota.checksum, chunk);
        _ota.received += static_cast<uint32_t>(chunk.size());
        sendAck(packet.seq);
        return;
    }

    if (packet.type == PacketType::OTA_END) {
        uint32_t size = 0;
        uint32_t checksum = 0;
        if (!Payloads::readU32(packet.payload, 1, size) ||
            !Payloads::readU32(packet.payload, 5, checksum) ||
            size != _ota.received ||
            checksum != _ota.checksum ||
            checksum != _ota.expectedChecksum ||
            !endOtaInstall()) {
            _ota.active = false;
            abortOtaInstall();
            sendNack(packet.seq, "ota verification failed");
            return;
        }
        _ota.active = false;
        sendAck(packet.seq);
        log("OTA image installed and verified. Rebooting.");
#ifdef ARDUINO
        delay(250);
        esp_restart();
#endif
    }
}

bool SlaveController::beginOtaInstall(uint32_t size)
{
#ifdef ARDUINO
    if (size == 0) return false;
    return Update.begin(size, U_FLASH);
#else
    (void)size;
    return true;
#endif
}

bool SlaveController::writeOtaChunk(const std::vector<uint8_t> &chunk)
{
#ifdef ARDUINO
    if (chunk.empty()) return true;
    return Update.write(const_cast<uint8_t *>(chunk.data()), chunk.size()) == chunk.size();
#else
    (void)chunk;
    return true;
#endif
}

bool SlaveController::endOtaInstall()
{
#ifdef ARDUINO
    return Update.end(true);
#else
    return true;
#endif
}

void SlaveController::abortOtaInstall()
{
#ifdef ARDUINO
    if (Update.isRunning()) {
        Update.abort();
    }
#endif
    _ota.active = false;
    _ota.installReady = false;
}
