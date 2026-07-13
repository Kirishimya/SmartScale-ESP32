#include <models/NetworkTable.h>
#include <algorithm>
#include <cstdint>

NetworkTable::NetworkTable(size_t capacity) : _cap(capacity) {}

bool NetworkTable::addOrUpdate(const NodeEntry &e) {
  for (auto &n : _nodes) {
    if (n.nodeId == e.nodeId) { n = e; return true; }
    if (n.mac == e.mac) { n = e; return true; }
  }
  if (_nodes.size() < _cap) { _nodes.push_back(e); return true; }
  // replace oldest
  auto it = std::min_element(_nodes.begin(), _nodes.end(), [](const NodeEntry &a, const NodeEntry &b){ return a.lastSeen < b.lastSeen; });
  if (it != _nodes.end()) *it = e;
  return true;
}

std::optional<NodeEntry> NetworkTable::findById(uint16_t id) const {
  for (const auto &n : _nodes) if (n.nodeId == id) return n;
  return {};
}

std::optional<NodeEntry> NetworkTable::findByMac(const std::array<uint8_t,6> &mac) const {
  for (const auto &n : _nodes) if (n.mac == mac) return n;
  return {};
}

std::vector<NodeEntry> NetworkTable::all() const { return _nodes; }

bool NetworkTable::setActive(uint16_t id, bool active) {
  for (auto &node : _nodes) {
    if (node.nodeId == id) {
      node.active = active;
      return true;
    }
  }
  return false;
}

bool NetworkTable::markSeen(uint16_t id, uint32_t nowMs) {
  for (auto &node : _nodes) {
    if (node.nodeId == id) {
      node.lastSeen = nowMs;
      node.active = true;
      node.consecutiveFailures = 0;
      return true;
    }
  }
  return false;
}

bool NetworkTable::recordSuccess(uint16_t id, uint32_t nowMs) {
  return markSeen(id, nowMs);
}

bool NetworkTable::recordFailure(uint16_t id) {
  for (auto &node : _nodes) {
    if (node.nodeId == id) {
      if (node.consecutiveFailures != UINT8_MAX) node.consecutiveFailures += 1;
      if (node.consecutiveFailures >= 3) node.active = false;
      return true;
    }
  }
  return false;
}

size_t NetworkTable::markInactiveSince(uint32_t nowMs, uint32_t timeoutMs) {
  size_t changed = 0;
  for (auto &node : _nodes) {
    if (node.active && nowMs - node.lastSeen > timeoutMs) {
      node.active = false;
      ++changed;
    }
  }
  return changed;
}
