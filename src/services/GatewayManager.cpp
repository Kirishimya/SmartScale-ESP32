#include <services/GatewayManager.h>

#include <models/Payloads.h>

#include <cstdio>
#include <sstream>

namespace {
std::string typeName(PacketType type) {
  switch (type) {
    case PacketType::SENSOR_DATA: return "sensor_data";
    case PacketType::DIAGNOSTIC: return "diagnostic";
    case PacketType::HEARTBEAT: return "heartbeat";
    default: return "packet";
  }
}

std::string jsonEscape(const std::string &text) {
  std::string out;
  out.reserve(text.size());
  for (char c : text) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out += c; break;
    }
  }
  return out;
}
}

GatewayManager::GatewayManager(size_t capacity)
    : _queue(capacity) {}

bool GatewayManager::begin(const char *storageNamespace) {
  return _queue.begin(storageNamespace);
}

bool GatewayManager::enqueuePacket(const Packet &packet, uint16_t nodeId, uint64_t receivedAtMs) {
  const std::string record = encodeRecord(packet, nodeId, receivedAtMs);
  return _queue.push(std::vector<uint8_t>(record.begin(), record.end()));
}

bool GatewayManager::popRecord(std::string &record) {
  std::vector<uint8_t> bytes;
  if (!_queue.pop(bytes)) return false;
  record.assign(bytes.begin(), bytes.end());
  return true;
}

size_t GatewayManager::pending() const {
  return _queue.size();
}

std::string GatewayManager::encodeRecord(
    const Packet &packet, uint16_t nodeId, uint64_t receivedAtMs) const {
  std::ostringstream out;
  out << "{\"schema\":\"smartscale.gateway.v1\","
      << "\"type\":\"" << typeName(packet.type) << "\","
      << "\"node_id\":" << nodeId << ","
      << "\"seq\":" << packet.seq << ","
      << "\"received_at_ms\":" << receivedAtMs;

  if (packet.type == PacketType::SENSOR_DATA) {
    Payloads::SensorData data;
    if (Payloads::decodeSensorData(packet.payload, data)) {
      out << ",\"weight_mg\":" << data.weightMg
          << ",\"unit_weight_mg\":" << data.unitWeightMg
          << ",\"estimated_units_milli\":" << data.estimatedUnitsMilli
          << ",\"sample_time_ms\":" << data.sampleTimeMs;
    }
  } else if (packet.type == PacketType::DIAGNOSTIC) {
    out << ",\"message\":\"" << jsonEscape(Payloads::decodeText(packet.payload)) << "\"";
  }

  out << "}";
  return out.str();
}
