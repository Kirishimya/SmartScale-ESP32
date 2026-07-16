#ifndef GATEWAY_MANAGER_H
#define GATEWAY_MANAGER_H

#include <models/Packet.h>
#include <storage/FlashQueue.h>

#include <cstdint>
#include <string>

class GatewayManager {
public:
  explicit GatewayManager(size_t capacity = 128);

  /// Initialize the persistent gateway queue.
  bool begin(const char *storageNamespace = "gateway");
  /// Convert a packet into a JSONL gateway record and retain it in flash/RAM.
  bool enqueuePacket(const Packet &packet, uint16_t nodeId, uint64_t receivedAtMs);
  /// Pop the oldest JSONL record for forwarding by a Raspberry/MQTT/HTTP bridge.
  bool popRecord(std::string &record);
  /// Number of retained gateway records waiting to be forwarded.
  size_t pending() const;

private:
  FlashQueue _queue;

  std::string encodeRecord(const Packet &packet, uint16_t nodeId, uint64_t receivedAtMs) const;
};

#endif // GATEWAY_MANAGER_H
