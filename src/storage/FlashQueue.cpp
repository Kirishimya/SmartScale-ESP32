#include <storage/FlashQueue.h>
#include <models/Packet.h>
#include <algorithm>
#include <cstdio>
#ifdef ARDUINO
#include <Preferences.h>
#endif

namespace {
constexpr uint32_t kRecordMagic = 0x53515458; // "SQTX"
constexpr size_t kRecordHeaderSize = 16;
constexpr size_t kMaxStoredItemSize = 2048;

void writeU32(std::vector<uint8_t> &out, uint32_t value) {
  out.push_back(static_cast<uint8_t>(value >> 24));
  out.push_back(static_cast<uint8_t>(value >> 16));
  out.push_back(static_cast<uint8_t>(value >> 8));
  out.push_back(static_cast<uint8_t>(value));
}

uint32_t readU32(const uint8_t *data) {
  return (uint32_t(data[0]) << 24) | (uint32_t(data[1]) << 16) |
         (uint32_t(data[2]) << 8) | uint32_t(data[3]);
}
}

FlashQueue::FlashQueue(size_t capacity) : _cap(capacity) {}

bool FlashQueue::begin(const char *storageNamespace) {
  _namespace = storageNamespace ? storageNamespace : "slave_tx";
#ifdef ARDUINO
  _persistent = true;
  return restore();
#else
  _persistent = false;
  return true;
#endif
}

bool FlashQueue::push(const std::vector<uint8_t> &item) {
  return push(item, {}, 0);
}

bool FlashQueue::push(const std::vector<uint8_t> &item, const std::array<uint8_t, 6> &destination, uint32_t sequence) {
  if (_buf.size() >= _cap) return false;
  if (item.size() > kMaxStoredItemSize) return false;
  QueuedPacket packet;
  packet.bytes = item;
  packet.destination = destination;
  packet.sequence = sequence;
  packet.firstAttemptAtMs = 0;
  packet.lastAttemptAtMs = 0;
  packet.retries = 0;
  packet.pending = true;
  packet.attempted = false;
  _buf.push_back(packet);
  if (persist()) return true;
  _buf.pop_back();
  return false;
}

bool FlashQueue::pop(std::vector<uint8_t> &out) {
  if (_buf.empty()) return false;
  out = _buf.front().bytes;
  _buf.erase(_buf.begin());
  return persist();
}

bool FlashQueue::popReady(std::vector<uint8_t> &out, uint32_t nowMs, uint32_t retryIntervalMs) {
  QueuedPacket packet;
  if (!popReady(packet, nowMs, retryIntervalMs)) return false;
  out = packet.bytes;
  return true;
}

bool FlashQueue::popReady(QueuedPacket &out, uint32_t nowMs, uint32_t retryIntervalMs) {
  if (_buf.empty()) return false;

  QueuedPacket &packet = _buf.front();
  if (!packet.pending) {
    _buf.erase(_buf.begin());
    return false;
  }

  if (!packet.attempted) {
    packet.attempted = true;
    packet.firstAttemptAtMs = nowMs;
    packet.lastAttemptAtMs = nowMs;
  } else {
    const uint8_t shift = packet.retries > 5 ? 5 : packet.retries;
    const uint32_t interval = retryIntervalMs << shift;
    if (nowMs - packet.lastAttemptAtMs < interval) return false;
    if (packet.retries != UINT8_MAX) packet.retries += 1;
    packet.lastAttemptAtMs = nowMs;
  }

  out = packet;
  return true;
}

void FlashQueue::markAcked() {
  if (_buf.empty()) return;
  _buf.front().pending = false;
  _buf.erase(_buf.begin());
  persist();
}

bool FlashQueue::markAcked(uint32_t sequence) {
  if (_buf.empty() || _buf.front().sequence != sequence) return false;
  markAcked();
  return true;
}

bool FlashQueue::empty() const { return _buf.empty(); }
size_t FlashQueue::size() const { return _buf.size(); }

bool FlashQueue::persist() {
  if (!_persistent) return true;
#ifdef ARDUINO
  Preferences preferences;
  if (!preferences.begin(_namespace.c_str(), false)) return false;
  preferences.clear();
  preferences.putUShort("count", static_cast<uint16_t>(_buf.size()));
  for (size_t i = 0; i < _buf.size(); ++i) {
    const auto &packet = _buf[i];
    if (packet.bytes.size() > UINT16_MAX) {
      preferences.end();
      return false;
    }
    std::vector<uint8_t> record;
    record.reserve(kRecordHeaderSize + packet.bytes.size());
    writeU32(record, kRecordMagic);
    writeU32(record, packet.sequence);
    record.insert(record.end(), packet.destination.begin(), packet.destination.end());
    const uint16_t length = static_cast<uint16_t>(packet.bytes.size());
    record.push_back(static_cast<uint8_t>(length >> 8));
    record.push_back(static_cast<uint8_t>(length));
    record.insert(record.end(), packet.bytes.begin(), packet.bytes.end());
    char key[8];
    snprintf(key, sizeof(key), "p%u", static_cast<unsigned>(i));
    if (preferences.putBytes(key, record.data(), record.size()) != record.size()) {
      preferences.end();
      return false;
    }
  }
  preferences.end();
#endif
  return true;
}

bool FlashQueue::restore() {
#ifdef ARDUINO
  Preferences preferences;
  if (!preferences.begin(_namespace.c_str(), false)) return false;
  const uint16_t count = preferences.getUShort("count", 0);
  if (count > _cap) {
    preferences.end();
    return false;
  }
  _buf.clear();
  for (uint16_t i = 0; i < count; ++i) {
    char key[8];
    snprintf(key, sizeof(key), "p%u", static_cast<unsigned>(i));
    const size_t size = preferences.getBytesLength(key);
    if (size < kRecordHeaderSize || size > kRecordHeaderSize + kMaxStoredItemSize) continue;
    std::vector<uint8_t> record(size);
    if (preferences.getBytes(key, record.data(), size) != size) continue;
    const uint16_t length = (uint16_t(record[14]) << 8) | record[15];
    if (readU32(record.data()) != kRecordMagic ||
        length + kRecordHeaderSize != record.size()) continue;
    QueuedPacket packet;
    packet.sequence = readU32(record.data() + 4);
    std::copy_n(record.data() + 8, packet.destination.size(), packet.destination.begin());
    packet.bytes.assign(record.begin() + kRecordHeaderSize, record.end());
    _buf.push_back(std::move(packet));
  }
  preferences.end();
#endif
  return true;
}
