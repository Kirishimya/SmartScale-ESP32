#include <services/DiagnosticManager.h>
#include <network/NetworkManager.h>
#include <iostream>

DiagnosticManager::DiagnosticManager(NetworkManager &network, NetworkTable &table)
  : _network(network), _table(table), _lastReport(0), _intervalMs(15000) {}

bool DiagnosticManager::begin() {
  return true;
}

void DiagnosticManager::update(uint32_t now_ms) {
  if (now_ms - _lastReport < _intervalMs) return;
  _lastReport = now_ms;
  for (auto &snapshot : _snapshots) {
    Packet p;
    p.type = PacketType::DIAGNOSTIC;
    p.dst_id = snapshot.nodeId;
    p.payload.reserve(16);
    const uint8_t *ptr = reinterpret_cast<const uint8_t*>(&snapshot.battery);
    p.payload.insert(p.payload.end(), ptr, ptr + sizeof(snapshot.battery));
    ptr = reinterpret_cast<const uint8_t*>(&snapshot.temperature);
    p.payload.insert(p.payload.end(), ptr, ptr + sizeof(snapshot.temperature));
    uint32_t uptimeMs = static_cast<uint32_t>(snapshot.uptimeSec * 1000);
    ptr = reinterpret_cast<const uint8_t*>(&uptimeMs);
    p.payload.insert(p.payload.end(), ptr, ptr + sizeof(uptimeMs));
    auto opt = _table.findById(snapshot.nodeId);
    if (opt.has_value()) {
      _network.send(p, opt->mac);
    }
  }
}

void DiagnosticManager::captureNodeHealth(uint16_t nodeId, float battery, float temperature, float uptimeSec) {
  _snapshots.push_back({nodeId, battery, temperature, uptimeSec});
}

bool DiagnosticManager::sendDiagnostic(uint16_t nodeId, const std::string &message) {
  auto opt = _table.findById(nodeId);
  if (!opt.has_value()) return false;
  Packet p;
  p.type = PacketType::DIAGNOSTIC;
  p.dst_id = nodeId;
  p.payload.assign(message.begin(), message.end());
  return _network.send(p, opt->mac);
}
