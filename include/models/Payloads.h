#ifndef PAYLOADS_H
#define PAYLOADS_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Payloads {

struct SensorData {
  static constexpr uint8_t kSchemaVersion = 1;
  int32_t weightMg = 0;
  int32_t unitWeightMg = 0;
  uint32_t estimatedUnitsMilli = 0;
  uint64_t sampleTimeMs = 0;
};

struct TimeSync {
  static constexpr uint8_t kSchemaVersion = 1;
  uint64_t epochMs = 0;
  uint64_t monotonicMs = 0;
};

enum class CommandId : uint8_t {
  Tare = 1,
  Reboot = 2,
  SendDiagnostic = 3,
  Rediscover = 4
};

enum class ConfigId : uint8_t {
  HeartbeatIntervalMs = 1,
  MasterNodeId = 2
};

void appendU16(std::vector<uint8_t> &payload, uint16_t value);
void appendU32(std::vector<uint8_t> &payload, uint32_t value);
void appendU64(std::vector<uint8_t> &payload, uint64_t value);
void appendI32(std::vector<uint8_t> &payload, int32_t value);
bool readU16(const std::vector<uint8_t> &payload, size_t offset, uint16_t &value);
bool readU32(const std::vector<uint8_t> &payload, size_t offset, uint32_t &value);
bool readU64(const std::vector<uint8_t> &payload, size_t offset, uint64_t &value);
bool readI32(const std::vector<uint8_t> &payload, size_t offset, int32_t &value);

std::vector<uint8_t> encodeSensorData(const SensorData &data);
bool decodeSensorData(const std::vector<uint8_t> &payload, SensorData &data);

std::vector<uint8_t> encodeTimeSync(const TimeSync &sync);
bool decodeTimeSync(const std::vector<uint8_t> &payload, TimeSync &sync);

std::vector<uint8_t> encodeAck(uint32_t sequence);
bool decodeAck(const std::vector<uint8_t> &payload, uint32_t &sequence);

std::vector<uint8_t> encodeText(const std::string &text);
std::string decodeText(const std::vector<uint8_t> &payload);

} // namespace Payloads

#endif // PAYLOADS_H
