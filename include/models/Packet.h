#ifndef PACKET_H
#define PACKET_H

#include <cstdint>
#include <vector>
#include <array>

enum class PacketType : uint8_t {
  DISCOVERY = 0x01,
  JOIN_REQUEST = 0x02,
  JOIN_ACCEPT = 0x03,
  JOIN_DENY = 0x04,
  NODE_LIST = 0x05,
  HEARTBEAT = 0x06,
  SENSOR_DATA = 0x07,
  COMMAND = 0x08,
  ACK = 0x09,
  NACK = 0x0A,
  ERROR = 0x0B,
  PING = 0x0C,
  PONG = 0x0D,
  TIME_SYNC = 0x0E,
  CONFIG = 0x0F,
  OTA_BEGIN = 0x10,
  OTA_DATA = 0x11,
  OTA_END = 0x12,
  DIAGNOSTIC = 0x13
};

struct Packet {
  static constexpr uint8_t kProtocolVersion = 1;
  // ESP-NOW frames are limited to 250 bytes. The protocol envelope and CRC
  // consume 36 bytes, so keep a conservative limit for application data.
  static constexpr size_t kMaxPayloadSize = 200;

  uint8_t version = kProtocolVersion;
  PacketType type = PacketType::SENSOR_DATA;
  std::array<uint8_t,6> src_mac{};
  std::array<uint8_t,6> dst_mac{};
  uint16_t src_id = 0;
  uint16_t dst_id = 0;
  uint32_t seq = 0;
  uint64_t ts = 0;
  std::vector<uint8_t> payload;
};

#endif // PACKET_H
