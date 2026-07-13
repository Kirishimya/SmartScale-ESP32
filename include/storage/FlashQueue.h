#ifndef FLASHQUEUE_H
#define FLASHQUEUE_H

#include <vector>
#include <cstddef>
#include <cstdint>
#include <array>
#include <string>

struct QueuedPacket {
  std::vector<uint8_t> bytes;
  std::array<uint8_t, 6> destination{};
  uint32_t sequence = 0;
  uint32_t firstAttemptAtMs = 0;
  uint32_t lastAttemptAtMs = 0;
  uint8_t retries = 0;
  bool pending = true;
  bool attempted = false;
};

class FlashQueue {
public:
  explicit FlashQueue(size_t capacity = 128);
  // Loads previously acknowledged-until data from NVS. Host builds keep the
  // same API but use only RAM.
  bool begin(const char *storageNamespace = "slave_tx");
  bool push(const std::vector<uint8_t> &item);
  bool push(const std::vector<uint8_t> &item, const std::array<uint8_t, 6> &destination, uint32_t sequence);
  bool pop(std::vector<uint8_t> &out);
  bool popReady(std::vector<uint8_t> &out, uint32_t nowMs, uint32_t retryIntervalMs);
  bool popReady(QueuedPacket &out, uint32_t nowMs, uint32_t retryIntervalMs);
  void markAcked();
  bool markAcked(uint32_t sequence);
  bool empty() const;
  size_t size() const;
private:
  size_t _cap;
  std::vector<QueuedPacket> _buf;
  std::string _namespace;
  bool _persistent = false;
  bool persist();
  bool restore();
};

#endif // FLASHQUEUE_H
