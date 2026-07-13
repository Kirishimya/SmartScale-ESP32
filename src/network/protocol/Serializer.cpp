#include <network/protocol/Serializer.h>
#include <cstring>
#include <optional>
#include <vector>
#include <cstdint>

using namespace std;

static void write_u16(vector<uint8_t> &out, uint16_t v) {
  out.push_back((v >> 8) & 0xFF);
  out.push_back(v & 0xFF);
}

static void write_u32(std::vector<uint8_t> &out, uint32_t v) {
  out.push_back((v >> 24) & 0xFF);
  out.push_back((v >> 16) & 0xFF);
  out.push_back((v >> 8) & 0xFF);
  out.push_back(v & 0xFF);
}

static void write_u64(std::vector<uint8_t> &out, uint64_t v) {
  for (int i = 7; i >= 0; --i) out.push_back((v >> (8*i)) & 0xFF);
}

static uint32_t crc32(const uint8_t *data, size_t len) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (int j = 0; j < 8; ++j) {
      if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320u;
      else crc = (crc >> 1);
    }
  }
  return ~crc;
}

std::vector<uint8_t> Serializer::serialize(const Packet &p) {
  if (p.payload.size() > Packet::kMaxPayloadSize) return {};
  std::vector<uint8_t> out;
  out.push_back(p.version);
  out.push_back(static_cast<uint8_t>(p.type));
  out.insert(out.end(), p.src_mac.begin(), p.src_mac.end());
  out.insert(out.end(), p.dst_mac.begin(), p.dst_mac.end());
  write_u16(out, p.src_id);
  write_u16(out, p.dst_id);
  write_u32(out, p.seq);
  write_u64(out, p.ts);
  write_u16(out, static_cast<uint16_t>(p.payload.size()));
  if (!p.payload.empty()) out.insert(out.end(), p.payload.begin(), p.payload.end());
  uint32_t crc = crc32(out.data(), out.size());
  write_u32(out, crc);
  return out;
}

static uint16_t read_u16(const uint8_t *ptr) {
  return (uint16_t(ptr[0]) << 8) | uint16_t(ptr[1]);
}

static uint32_t read_u32(const uint8_t *ptr) {
  return (uint32_t(ptr[0])<<24) | (uint32_t(ptr[1])<<16) | (uint32_t(ptr[2])<<8) | uint32_t(ptr[3]);
}

static uint64_t read_u64(const uint8_t *ptr) {
  uint64_t v = 0;
  for (int i = 0; i < 8; ++i) v = (v<<8) | ptr[i];
  return v;
}

std::optional<Packet> Serializer::parse(const uint8_t *data, size_t len) {
  if (!data || len < 32 + 4) return {};
  Packet p;
  size_t idx = 0;
  p.version = data[idx++];
  p.type = static_cast<PacketType>(data[idx++]);
  memcpy(p.src_mac.data(), data+idx, 6); idx += 6;
  memcpy(p.dst_mac.data(), data+idx, 6); idx += 6;
  p.src_id = read_u16(data+idx); idx += 2;
  p.dst_id = read_u16(data+idx); idx += 2;
  p.seq = read_u32(data+idx); idx += 4;
  p.ts = read_u64(data+idx); idx += 8;
  uint16_t payload_len = read_u16(data+idx); idx += 2;
  if (payload_len > Packet::kMaxPayloadSize) return {};
  if (idx + payload_len + 4 != len) return {};
  if (payload_len) p.payload.assign(data+idx, data+idx+payload_len);
  idx += payload_len;
  uint32_t expected = read_u32(data+idx);
  uint32_t crc = crc32(data, idx);
  if (crc != expected) return {};
  if (p.version != Packet::kProtocolVersion) return {};
  if (p.payload.size() != payload_len) return {};
  if (p.type < PacketType::DISCOVERY || p.type > PacketType::DIAGNOSTIC) return {};
  return p;
}
