#include <network/PollingManager.h>
#include <network/NetworkManager.h>
#include <algorithm>

PollingManager::PollingManager(NetworkManager &network, NetworkTable &table)
  : _network(network), _table(table), _lastRound(0), _intervalMs(5000) {}

void PollingManager::begin() {
  // nothing for stub
}

void PollingManager::update(uint32_t now_ms) {
  if (_lastRound == 0) _lastRound = now_ms;
  if (now_ms - _lastRound >= _intervalMs) {
    _lastRound = now_ms;
    auto nodes = _table.all();
    for (const auto &n : nodes) {
      if (!n.active) continue;
      if (!pollNode(n)) _table.recordFailure(n.nodeId);
    }
  }
}

bool PollingManager::pollNode(const NodeEntry &node) {
  Packet p;
  p.type = PacketType::PING;
  p.dst_id = node.nodeId;
  return _network.send(p, node.mac);
}
