#ifndef PAYLOADS_H
#define PAYLOADS_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Payloads {

/// Portable sensor sample payload used by SENSOR_DATA.
///
/// Values are intentionally integer/fixed-point and big-endian on the wire so
/// packets are stable across MCU ABI, compiler, and host gateway code.
struct SensorData {
  static constexpr uint8_t kSchemaVersion = 1;
  int32_t weightMg = 0;
  int32_t unitWeightMg = 0;
  uint32_t estimatedUnitsMilli = 0;
  uint64_t sampleTimeMs = 0;
};

/// Master-driven clock synchronization payload.
struct TimeSync {
  static constexpr uint8_t kSchemaVersion = 1;
  uint64_t epochMs = 0;
  uint64_t monotonicMs = 0;
};

/// Runtime commands accepted by slave nodes.
enum class CommandId : uint8_t {
  Tare = 1,
  Reboot = 2,
  SendDiagnostic = 3,
  Rediscover = 4
};

/// Persistable runtime configuration keys accepted by slave nodes.
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

/// Encode a SENSOR_DATA payload.
std::vector<uint8_t> encodeSensorData(const SensorData &data);
/// Decode a SENSOR_DATA payload. Returns false on schema/version mismatch.
bool decodeSensorData(const std::vector<uint8_t> &payload, SensorData &data);

/// Encode a TIME_SYNC payload.
std::vector<uint8_t> encodeTimeSync(const TimeSync &sync);
/// Decode a TIME_SYNC payload. Returns false on schema/version mismatch.
bool decodeTimeSync(const std::vector<uint8_t> &payload, TimeSync &sync);

/// Encode the acknowledged sequence number carried by ACK/NACK packets.
std::vector<uint8_t> encodeAck(uint32_t sequence);
/// Decode the acknowledged sequence number carried by ACK/NACK packets.
bool decodeAck(const std::vector<uint8_t> &payload, uint32_t &sequence);

std::vector<uint8_t> encodeText(const std::string &text);
std::string decodeText(const std::vector<uint8_t> &payload);

} // namespace Payloads

#endif // PAYLOADS_H
