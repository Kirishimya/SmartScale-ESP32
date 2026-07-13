#ifndef NETWORK_TABLE_H
#define NETWORK_TABLE_H

#include <cstdint>
#include <vector>
#include <array>
#include <optional>

struct NodeEntry {
  uint16_t nodeId = 0;
  std::array<uint8_t,6> mac{};
  uint32_t lastSeen = 0; // millis
  uint8_t rssi = 0;
  bool active = false;
  uint8_t capabilities = 0;
  uint8_t consecutiveFailures = 0;
};

class NetworkTable {
public:
  NetworkTable(size_t capacity = 64);
  bool addOrUpdate(const NodeEntry &e);
  std::optional<NodeEntry> findById(uint16_t id) const;
  std::optional<NodeEntry> findByMac(const std::array<uint8_t,6> &mac) const;
  std::vector<NodeEntry> all() const;
  bool setActive(uint16_t id, bool active);
  bool markSeen(uint16_t id, uint32_t nowMs);
  bool recordSuccess(uint16_t id, uint32_t nowMs);
  bool recordFailure(uint16_t id);
  size_t markInactiveSince(uint32_t nowMs, uint32_t timeoutMs);
private:
  size_t _cap;
  std::vector<NodeEntry> _nodes;
};

#endif // NETWORK_TABLE_H
