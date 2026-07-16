#include <network/OTAManager.h>

#include <models/Payloads.h>
#include <network/NetworkManager.h>
#include <services/Logger.h>

#include <algorithm>
#include <string>

namespace {
constexpr uint32_t kRetryMs = 1200;
constexpr uint8_t kMaxRetries = 5;
constexpr size_t kChunkSize = 96;
}

OTAManager::OTAManager(NetworkManager &network, NetworkTable &table)
  : _network(network),
    _table(table),
    _active(false),
    _targetNode(0),
    _offset(0),
    _ackedOffset(0),
    _stage(Stage::Idle),
    _checksum(0),
    _waitingSequence(0),
    _lastSendAt(0),
    _retries(0) {}

bool OTAManager::begin() { return true; }

bool OTAManager::startOTA(uint16_t nodeId, const std::vector<uint8_t> &image) {
  if (image.empty()) return false;
  if (!_table.findById(nodeId).has_value()) return false;

  _active = true;
  _targetNode = nodeId;
  _image = image;
  _offset = 0;
  _ackedOffset = 0;
  _stage = Stage::Begin;
  _checksum = checksum(image);
  _waitingSequence = 0;
  _lastSendAt = 0;
  _retries = 0;

  Logger::instance().info(
      "ota",
      "Starting OTA to node " + std::to_string(nodeId) +
          " image size=" + std::to_string(image.size()));
  return true;
}

void OTAManager::update(uint32_t now_ms) {
  if (!_active) return;
  if (!_table.findById(_targetNode).has_value()) {
    _stage = Stage::Failed;
  }

  if (_stage == Stage::Failed) {
    Logger::instance().warn("ota", "OTA failed for node " + std::to_string(_targetNode));
    _active = false;
    _stage = Stage::Idle;
    return;
  }

  if (_stage == Stage::Complete) {
    Logger::instance().info("ota", "OTA complete for node " + std::to_string(_targetNode));
    _active = false;
    _stage = Stage::Idle;
    return;
  }

  if (_waitingSequence != 0) {
    if (now_ms - _lastSendAt < kRetryMs) return;
    if (_retries >= kMaxRetries) {
      _stage = Stage::Failed;
      return;
    }
    ++_retries;
  }

  sendCurrent(now_ms);
}

void OTAManager::handleAck(uint16_t nodeId, uint32_t sequence) {
  if (!_active || nodeId != _targetNode || sequence != _waitingSequence) return;
  _waitingSequence = 0;
  _retries = 0;
  _lastSendAt = 0;

  switch (_stage) {
    case Stage::Begin:
      _stage = Stage::Data;
      break;
    case Stage::Data:
      _ackedOffset = _offset;
      if (_ackedOffset >= _image.size()) _stage = Stage::End;
      break;
    case Stage::End:
      _stage = Stage::Complete;
      break;
    default:
      break;
  }
}

void OTAManager::handleNack(uint16_t nodeId, uint32_t sequence) {
  if (!_active || nodeId != _targetNode || sequence != _waitingSequence) return;
  _waitingSequence = 0;
  _lastSendAt = 0;
  if (_retries >= kMaxRetries) {
    _stage = Stage::Failed;
  } else {
    ++_retries;
  }
}

bool OTAManager::sendCurrent(uint32_t now_ms) {
  auto node = _table.findById(_targetNode);
  if (!node.has_value()) return false;

  Packet packet;
  packet.dst_id = _targetNode;
  packet.payload.push_back(1);

  switch (_stage) {
    case Stage::Begin:
      packet.type = PacketType::OTA_BEGIN;
      Payloads::appendU32(packet.payload, static_cast<uint32_t>(_image.size()));
      Payloads::appendU32(packet.payload, _checksum);
      return sendPacket(packet, now_ms);

    case Stage::Data:
    {
      packet.type = PacketType::OTA_DATA;
      Payloads::appendU32(packet.payload, static_cast<uint32_t>(_ackedOffset));
      const size_t remaining = _image.size() - _ackedOffset;
      const size_t toSend = std::min(kChunkSize, remaining);
      packet.payload.insert(
          packet.payload.end(),
          _image.begin() + _ackedOffset,
          _image.begin() + _ackedOffset + toSend);
      _offset = _ackedOffset + toSend;
      return sendPacket(packet, now_ms);
    }

    case Stage::End:
      packet.type = PacketType::OTA_END;
      Payloads::appendU32(packet.payload, static_cast<uint32_t>(_image.size()));
      Payloads::appendU32(packet.payload, _checksum);
      return sendPacket(packet, now_ms);

    default:
      return false;
  }
}

bool OTAManager::sendPacket(Packet &packet, uint32_t now_ms) {
  auto node = _table.findById(_targetNode);
  if (!node.has_value()) return false;

  auto encoded = _network.encode(packet, node->mac);
  if (!encoded.has_value() || !_network.send(*encoded)) {
    return false;
  }
  _waitingSequence = encoded->sequence;
  _lastSendAt = now_ms;
  return true;
}

uint32_t OTAManager::checksum(const std::vector<uint8_t> &image) const {
  uint32_t hash = 2166136261UL;
  for (uint8_t byte : image) {
    hash ^= byte;
    hash *= 16777619UL;
  }
  return hash;
}
