#include <models/Payloads.h>

namespace Payloads {

void appendU16(std::vector<uint8_t> &payload, uint16_t value) {
  payload.push_back(static_cast<uint8_t>(value >> 8));
  payload.push_back(static_cast<uint8_t>(value));
}

void appendU32(std::vector<uint8_t> &payload, uint32_t value) {
  payload.push_back(static_cast<uint8_t>(value >> 24));
  payload.push_back(static_cast<uint8_t>(value >> 16));
  payload.push_back(static_cast<uint8_t>(value >> 8));
  payload.push_back(static_cast<uint8_t>(value));
}

void appendU64(std::vector<uint8_t> &payload, uint64_t value) {
  for (int shift = 56; shift >= 0; shift -= 8) {
    payload.push_back(static_cast<uint8_t>(value >> shift));
  }
}

void appendI32(std::vector<uint8_t> &payload, int32_t value) {
  appendU32(payload, static_cast<uint32_t>(value));
}

bool readU16(const std::vector<uint8_t> &payload, size_t offset, uint16_t &value) {
  if (offset + 2 > payload.size()) return false;
  value = (uint16_t(payload[offset]) << 8) | uint16_t(payload[offset + 1]);
  return true;
}

bool readU32(const std::vector<uint8_t> &payload, size_t offset, uint32_t &value) {
  if (offset + 4 > payload.size()) return false;
  value = (uint32_t(payload[offset]) << 24) |
          (uint32_t(payload[offset + 1]) << 16) |
          (uint32_t(payload[offset + 2]) << 8) |
          uint32_t(payload[offset + 3]);
  return true;
}

bool readU64(const std::vector<uint8_t> &payload, size_t offset, uint64_t &value) {
  if (offset + 8 > payload.size()) return false;
  value = 0;
  for (size_t i = 0; i < 8; ++i) {
    value = (value << 8) | uint64_t(payload[offset + i]);
  }
  return true;
}

bool readI32(const std::vector<uint8_t> &payload, size_t offset, int32_t &value) {
  uint32_t raw = 0;
  if (!readU32(payload, offset, raw)) return false;
  value = static_cast<int32_t>(raw);
  return true;
}

std::vector<uint8_t> encodeSensorData(const SensorData &data) {
  std::vector<uint8_t> payload;
  payload.reserve(22);
  payload.push_back(SensorData::kSchemaVersion);
  payload.push_back(0);
  appendI32(payload, data.weightMg);
  appendI32(payload, data.unitWeightMg);
  appendU32(payload, data.estimatedUnitsMilli);
  appendU64(payload, data.sampleTimeMs);
  return payload;
}

bool decodeSensorData(const std::vector<uint8_t> &payload, SensorData &data) {
  if (payload.size() != 22 || payload[0] != SensorData::kSchemaVersion) return false;
  return readI32(payload, 2, data.weightMg) &&
         readI32(payload, 6, data.unitWeightMg) &&
         readU32(payload, 10, data.estimatedUnitsMilli) &&
         readU64(payload, 14, data.sampleTimeMs);
}

std::vector<uint8_t> encodeTimeSync(const TimeSync &sync) {
  std::vector<uint8_t> payload;
  payload.reserve(17);
  payload.push_back(TimeSync::kSchemaVersion);
  appendU64(payload, sync.epochMs);
  appendU64(payload, sync.monotonicMs);
  return payload;
}

bool decodeTimeSync(const std::vector<uint8_t> &payload, TimeSync &sync) {
  if (payload.size() != 17 || payload[0] != TimeSync::kSchemaVersion) return false;
  return readU64(payload, 1, sync.epochMs) &&
         readU64(payload, 9, sync.monotonicMs);
}

std::vector<uint8_t> encodeAck(uint32_t sequence) {
  std::vector<uint8_t> payload;
  payload.reserve(4);
  appendU32(payload, sequence);
  return payload;
}

bool decodeAck(const std::vector<uint8_t> &payload, uint32_t &sequence) {
  return payload.size() == 4 && readU32(payload, 0, sequence);
}

std::vector<uint8_t> encodeText(const std::string &text) {
  return std::vector<uint8_t>(text.begin(), text.end());
}

std::string decodeText(const std::vector<uint8_t> &payload) {
  return std::string(payload.begin(), payload.end());
}

} // namespace Payloads
