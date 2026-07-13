#include <application/MasterController.h>
#include <cstring>
#include <iostream>

MasterController::MasterController(IEspNowDriver &driver)
  : _driver(driver), _network(driver, 1), _table(128), _poller(_network, _table), _ota(_network, _table), _diagnostics(_network, _table), _lastPoll(0), _lastWatchdog(0), _watchdogInterval(10000), _nextNodeId(2) {}

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
    _network.send(timeSync, mac);
    return;
  }

  if (packet.type == PacketType::SENSOR_DATA) {
    Packet ack;
    ack.type = PacketType::ACK;
    ack.dst_id = nodeId;
    ack.payload = {
      static_cast<uint8_t>((packet.seq >> 24) & 0xFF),
      static_cast<uint8_t>((packet.seq >> 16) & 0xFF),
      static_cast<uint8_t>((packet.seq >> 8) & 0xFF),
      static_cast<uint8_t>(packet.seq & 0xFF)};
    _network.send(ack, mac);
  }
}
