#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <atomic>
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include <drivers/EspNowDriver.h>
#include <models/Packet.h>

class NetworkManager {
public:
  using MacAddress = std::array<uint8_t, 6>;
  using PacketHandler = std::function<void(const Packet &, const MacAddress &)>;
  using DeliveryHandler = std::function<void(const MacAddress &, bool)>;

  struct EncodedPacket {
    std::vector<uint8_t> bytes;
    MacAddress destination{};
    uint32_t sequence = 0;
  };

  NetworkManager(IEspNowDriver &driver, uint16_t localNodeId);

  bool begin();
  void poll();
  void setPacketHandler(PacketHandler handler);
  void setDeliveryHandler(DeliveryHandler handler);
  void setLocalNodeId(uint16_t nodeId);
  uint16_t localNodeId() const;
  const MacAddress &localMac() const;
  uint64_t nowMillis() const;
  uint32_t droppedRx() const;

  std::optional<EncodedPacket> encode(Packet packet, const MacAddress &destination);
  bool send(const EncodedPacket &packet);
  bool send(Packet packet, const MacAddress &destination);
  bool broadcast(Packet packet);

  static MacAddress broadcastMac();
  static bool isBroadcast(const MacAddress &mac);

private:
  static constexpr uint8_t kRxQueueSize = 12;
  static constexpr uint8_t kTxQueueSize = 12;
  static constexpr size_t kMaxRawFrameSize = 250;
  static constexpr size_t kMaxPeerSequences = 128;

  struct RawFrame {
    std::array<uint8_t, kMaxRawFrameSize> bytes{};
    MacAddress sender{};
    size_t len = 0;
  };

  struct DeliveryEvent {
    MacAddress destination{};
    bool success = false;
  };

  struct PeerSequence {
    MacAddress mac{};
    uint32_t lastSequence = 0;
    uint32_t lastSeen = 0;
  };

  IEspNowDriver &_driver;
  uint16_t _localNodeId;
  uint32_t _nextSequence;
  MacAddress _localMac{};
  PacketHandler _handler;
  DeliveryHandler _deliveryHandler;
  std::array<RawFrame, kRxQueueSize> _rxQueue{};
  std::array<DeliveryEvent, kTxQueueSize> _txQueue{};
  std::atomic<uint8_t> _rxHead{0};
  std::atomic<uint8_t> _rxTail{0};
  std::atomic<uint8_t> _txHead{0};
  std::atomic<uint8_t> _txTail{0};
  std::atomic<uint32_t> _droppedRx{0};
  std::vector<PeerSequence> _peerSequences;

  void handleRaw(const uint8_t *data, size_t len, const uint8_t *mac);
  void handleDelivery(const uint8_t *mac, bool success);
  void processRawFrame(const RawFrame &frame);
  bool acceptIncomingSequence(const MacAddress &mac, uint32_t sequence, uint32_t nowMs);
};

#endif // NETWORK_MANAGER_H
