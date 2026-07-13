#include <application/SlaveController.h>

#include <application/slave/State.h>
#include <application/slave/StateFactory.h>
#include <services/Logger.h>

#include <cstring>
#include <iostream>
#include <chrono>

#ifdef ARDUINO
#include <Arduino.h>
#include <Preferences.h>

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

    pkt.payload.resize(sizeof(float) * 3);

    std::memcpy(
        pkt.payload.data(),
        &weight,
        sizeof(float));

    std::memcpy(
        pkt.payload.data() + sizeof(float),
        &weightPerUnit,
        sizeof(float));

    std::memcpy(
        pkt.payload.data() + sizeof(float) * 2,
        &estimatedUnits,
        sizeof(float));

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
            log("TIME_SYNC received.");

            requestStateTransition("READY");

            break;
        }

        case PacketType::ACK:
        {
            if (packet.payload.size() == 4) {
                const uint32_t acknowledged =
                    (uint32_t(packet.payload[0]) << 24) |
                    (uint32_t(packet.payload[1]) << 16) |
                    (uint32_t(packet.payload[2]) << 8) |
                    uint32_t(packet.payload[3]);
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
            // TODO
            // Command Dispatcher

            break;
        }

        case PacketType::CONFIG:
        {
            // TODO
            // ConfigManager

            break;
        }

        case PacketType::OTA_BEGIN:
        case PacketType::OTA_DATA:
        case PacketType::OTA_END:
        {
            // TODO
            // OTA Manager

            break;
        }

        default:
            break;
    }
}
