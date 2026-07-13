#include <network/NetworkManager.h>

#include <network/protocol/ProtocolValidator.h>
#include <network/protocol/Serializer.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <utility>

#ifdef ARDUINO
#include <Arduino.h>
#endif

NetworkManager::NetworkManager(IEspNowDriver &driver, uint16_t localNodeId)
    : _driver(driver), _localNodeId(localNodeId), _nextSequence(0) {}

bool NetworkManager::begin() {
  if (!_driver.begin()) return false;
  if (!_driver.localMac(_localMac.data())) return false;
  _driver.onReceive([this](const uint8_t *data, size_t len, const uint8_t *mac) {
    handleRaw(data, len, mac);
  });
  _driver.onSend([this](const uint8_t *mac, bool success) {
    handleDelivery(mac, success);
  });
  return true;
}

void NetworkManager::poll() {
  for (;;) {
    const uint8_t tail = _rxTail.load(std::memory_order_acquire);
    const uint8_t head = _rxHead.load(std::memory_order_acquire);
    if (tail == head) break;

    RawFrame frame = _rxQueue[tail];
    _rxTail.store(static_cast<uint8_t>((tail + 1) % kRxQueueSize), std::memory_order_release);
    processRawFrame(frame);
  }

  for (;;) {
    const uint8_t tail = _txTail.load(std::memory_order_acquire);
    const uint8_t head = _txHead.load(std::memory_order_acquire);
    if (tail == head) break;

    DeliveryEvent event = _txQueue[tail];
    _txTail.store(static_cast<uint8_t>((tail + 1) % kTxQueueSize), std::memory_order_release);
    if (_deliveryHandler) _deliveryHandler(event.destination, event.success);
  }
}

void NetworkManager::setPacketHandler(PacketHandler handler) {
  _handler = std::move(handler);
}

void NetworkManager::setDeliveryHandler(DeliveryHandler handler) {
  _deliveryHandler = std::move(handler);
}

void NetworkManager::setLocalNodeId(uint16_t nodeId) { _localNodeId = nodeId; }
uint16_t NetworkManager::localNodeId() const { return _localNodeId; }
const NetworkManager::MacAddress &NetworkManager::localMac() const { return _localMac; }
uint32_t NetworkManager::droppedRx() const { return _droppedRx.load(std::memory_order_relaxed); }

uint64_t NetworkManager::nowMillis() const {
#ifdef ARDUINO
  return ::millis();
#else
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
#endif
}

std::optional<NetworkManager::EncodedPacket> NetworkManager::encode(
    Packet packet, const MacAddress &destination) {
  if (!ProtocolValidator::isValidAddress(destination)) return {};

  packet.version = Packet::kProtocolVersion;
  packet.src_mac = _localMac;
  packet.dst_mac = destination;
  packet.src_id = _localNodeId;
  if (packet.seq == 0) packet.seq = ++_nextSequence;
  if (packet.ts == 0) packet.ts = nowMillis();
  if (!ProtocolValidator::isValidPacket(packet)) return {};

  EncodedPacket encoded;
  encoded.destination = destination;
  encoded.sequence = packet.seq;
  encoded.bytes = Serializer::serialize(packet);
  if (encoded.bytes.empty()) return {};
  return encoded;
}

bool NetworkManager::send(const EncodedPacket &packet) {
  if (packet.bytes.empty()) return false;
  return _driver.send(packet.bytes.data(), packet.bytes.size(), packet.destination.data());
}

bool NetworkManager::send(Packet packet, const MacAddress &destination) {
  auto encoded = encode(std::move(packet), destination);
  return encoded.has_value() && send(*encoded);
}

bool NetworkManager::broadcast(Packet packet) {
  return send(std::move(packet), broadcastMac());
}

NetworkManager::MacAddress NetworkManager::broadcastMac() {
  return {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
}

bool NetworkManager::isBroadcast(const MacAddress &mac) {
  return mac == broadcastMac();
}

void NetworkManager::handleRaw(const uint8_t *data, size_t len, const uint8_t *mac) {
  if (!data || !mac || len == 0 || len > kMaxRawFrameSize) {
    _droppedRx.fetch_add(1, std::memory_order_relaxed);
    return;
  }

  const uint8_t head = _rxHead.load(std::memory_order_relaxed);
  const uint8_t next = static_cast<uint8_t>((head + 1) % kRxQueueSize);
  if (next == _rxTail.load(std::memory_order_acquire)) {
    _droppedRx.fetch_add(1, std::memory_order_relaxed);
    return;
  }

  RawFrame &frame = _rxQueue[head];
  frame.len = len;
  std::memcpy(frame.bytes.data(), data, len);
  std::memcpy(frame.sender.data(), mac, frame.sender.size());
  _rxHead.store(next, std::memory_order_release);
}

void NetworkManager::handleDelivery(const uint8_t *mac, bool success) {
  if (!mac) return;

  const uint8_t head = _txHead.load(std::memory_order_relaxed);
  const uint8_t next = static_cast<uint8_t>((head + 1) % kTxQueueSize);
  if (next == _txTail.load(std::memory_order_acquire)) return;

  DeliveryEvent &event = _txQueue[head];
  std::memcpy(event.destination.data(), mac, event.destination.size());
  event.success = success;
  _txHead.store(next, std::memory_order_release);
}

void NetworkManager::processRawFrame(const RawFrame &frame) {
  auto parsed = Serializer::parse(frame.bytes.data(), frame.len);
  if (!parsed.has_value() || !ProtocolValidator::isValidPacket(*parsed)) return;

  // The radio-level sender must match the authenticated protocol envelope.
  if (parsed->src_mac != frame.sender) return;
  if (parsed->src_mac == broadcastMac()) return;
  if (parsed->dst_mac != _localMac && !isBroadcast(parsed->dst_mac)) return;
  if (parsed->seq == 0) return;
  if (!acceptIncomingSequence(frame.sender, parsed->seq, static_cast<uint32_t>(nowMillis()))) return;

  if (!_handler) return;
  _handler(*parsed, frame.sender);
}

bool NetworkManager::acceptIncomingSequence(
    const MacAddress &mac, uint32_t sequence, uint32_t nowMs) {
  if (sequence == 0) return false;

  for (auto &peer : _peerSequences) {
    if (peer.mac != mac) continue;
    const uint32_t delta = sequence - peer.lastSequence;
    if (delta == 0 || delta >= 0x80000000UL) return false;
    peer.lastSequence = sequence;
    peer.lastSeen = nowMs;
    return true;
  }

  PeerSequence peer;
  peer.mac = mac;
  peer.lastSequence = sequence;
  peer.lastSeen = nowMs;

  if (_peerSequences.size() < kMaxPeerSequences) {
    _peerSequences.push_back(peer);
    return true;
  }

  auto oldest = std::min_element(
      _peerSequences.begin(),
      _peerSequences.end(),
      [](const PeerSequence &a, const PeerSequence &b) {
        return a.lastSeen < b.lastSeen;
      });
  if (oldest != _peerSequences.end()) *oldest = peer;
  return true;
}
