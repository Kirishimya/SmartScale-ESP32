#ifndef GATEWAY_MANAGER_H
#define GATEWAY_MANAGER_H

#include <models/Packet.h>
#include <storage/FlashQueue.h>

#include <cstdint>
#include <string>

class GatewayManager {
public:
  explicit GatewayManager(size_t capacity = 128);

  bool begin(const char *storageNamespace = "gateway");
  bool enqueuePacket(const Packet &packet, uint16_t nodeId, uint64_t receivedAtMs);
  bool popRecord(std::string &record);
  size_t pending() const;

private:
  FlashQueue _queue;

  std::string encodeRecord(const Packet &packet, uint16_t nodeId, uint64_t receivedAtMs) const;
};

#endif // GATEWAY_MANAGER_H
