#include <application/MasterController.h>
#include <models/Payloads.h>
#include <services/Logger.h>
#include <cstring>
#include <iostream>

MasterController::MasterController(IEspNowDriver &driver)
  : _driver(driver), _network(driver, 1), _table(128), _poller(_network, _table), _ota(_network, _table), _diagnostics(_network, _table), _gateway(256), _lastPoll(0), _lastWatchdog(0), _watchdogInterval(10000), _nextNodeId(2) {}

bool MasterController::begin() {
  if (!_network.begin()) return false;
  _network.setPacketHandler([this](const Packet &packet, const NetworkManager::MacAddress &mac) {
    handlePacket(packet, mac);
  });
  _network.setDeliveryHandler([this](const NetworkManager::MacAddress &mac, bool success) {
    auto known = _table.findByMac(mac);
    if (!known.has_value()) return;
    const uint32_t now = static_cast<uint32_t>(_network.nowMillis());
    if (success) {
      _table.recordSuccess(known->nodeId, now);
    } else {
      _table.recordFailure(known->nodeId);
    }
  });
  _poller.begin();
  _ota.begin();
  _diagnostics.begin();
  if (!_gateway.begin("gateway")) {
    Logger::instance().warn("master", "Gateway buffer unavailable.");
  }
  return true;
}

void MasterController::loop() {
  _network.poll();

  const uint32_t now = static_cast<uint32_t>(_network.nowMillis());
  _poller.update(now);
  _ota.update(now);
  _diagnostics.update(now);
  if (now - _lastWatchdog >= _watchdogInterval) {
    _lastWatchdog = now;
    _table.markInactiveSince(now, 30000);
  }
}

uint16_t MasterController::nodeIdFor(const NetworkManager::MacAddress &mac) {
  auto known = _table.findByMac(mac);
  if (known.has_value()) return known->nodeId;

  while (_table.findById(_nextNodeId).has_value() || _nextNodeId == 0 || _nextNodeId == 1) {
    ++_nextNodeId;
    if (_nextNodeId == 0) _nextNodeId = 2;
  }
  return _nextNodeId++;
}

void MasterController::handlePacket(const Packet &packet, const NetworkManager::MacAddress &mac) {
  const uint32_t now = static_cast<uint32_t>(_network.nowMillis());
  const bool isJoin = packet.type == PacketType::DISCOVERY || packet.type == PacketType::JOIN_REQUEST;
  auto known = _table.findByMac(mac);

  if (!known.has_value() && !isJoin) {
    return;
  }

  if (known.has_value() && packet.src_id != 0 && packet.src_id != known->nodeId) {
    return;
  }

  const uint16_t nodeId = known.has_value() ? known->nodeId : nodeIdFor(mac);
  NodeEntry entry = known.value_or(NodeEntry{});
  entry.nodeId = nodeId;
  entry.mac = mac;
  entry.lastSeen = now;
  entry.active = true;
  entry.consecutiveFailures = 0;
  if (isJoin && !packet.payload.empty()) {
    entry.capabilities = packet.payload[0];
  }
  _table.addOrUpdate(entry);

  if (isJoin) {
    if (_onNodeDiscovered) _onNodeDiscovered(entry);

    Packet accepted;
    accepted.type = PacketType::JOIN_ACCEPT;
    accepted.dst_id = nodeId;
    if (!_network.send(accepted, mac)) return;

    Packet timeSync;
    timeSync.type = PacketType::TIME_SYNC;
    timeSync.dst_id = nodeId;
    const uint64_t nowMs = _network.nowMillis();
    timeSync.payload = Payloads::encodeTimeSync({nowMs, nowMs});
    _network.send(timeSync, mac);
    return;
  }

  if (packet.type == PacketType::SENSOR_DATA) {
    Payloads::SensorData sample;
    if (Payloads::decodeSensorData(packet.payload, sample)) {
      Logger::instance().debug(
          "master",
          "Sensor sample node=" + std::to_string(nodeId) +
              " weight_mg=" + std::to_string(sample.weightMg));
    }
    if (!_gateway.enqueuePacket(packet, nodeId, now)) {
      Logger::instance().warn("master", "Gateway buffer full; sensor sample was not retained.");
    }
    Packet ack;
    ack.type = PacketType::ACK;
    ack.dst_id = nodeId;
    ack.payload = Payloads::encodeAck(packet.seq);
    _network.send(ack, mac);
    return;
  }

  if (packet.type == PacketType::DIAGNOSTIC || packet.type == PacketType::HEARTBEAT) {
    if (!_gateway.enqueuePacket(packet, nodeId, now)) {
      Logger::instance().warn("master", "Gateway buffer full; packet was not retained.");
    }
    return;
  }

  if (packet.type == PacketType::ACK) {
    uint32_t sequence = 0;
    if (Payloads::decodeAck(packet.payload, sequence)) {
      _ota.handleAck(nodeId, sequence);
    }
    return;
  }

  if (packet.type == PacketType::NACK) {
    uint32_t sequence = 0;
    if (Payloads::decodeAck(packet.payload, sequence)) {
      _ota.handleNack(nodeId, sequence);
    }
  }
}
