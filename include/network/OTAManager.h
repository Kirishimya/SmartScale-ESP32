#ifndef OTAMANAGER_H
#define OTAMANAGER_H

#include <models/NetworkTable.h>
#include <models/Packet.h>
#include <cstddef>
#include <cstdint>
#include <vector>

class NetworkManager;

class OTAManager {
public:
  OTAManager(NetworkManager &network, NetworkTable &table);
  bool begin();
  bool startOTA(uint16_t nodeId, const std::vector<uint8_t> &image);
  void update(uint32_t now_ms);
  void handleAck(uint16_t nodeId, uint32_t sequence);
  void handleNack(uint16_t nodeId, uint32_t sequence);
private:
  enum class Stage : uint8_t {
    Idle,
    Begin,
    Data,
    End,
    Complete,
    Failed
  };

  NetworkManager &_network;
  NetworkTable &_table;
  bool _active;
  uint16_t _targetNode;
  size_t _offset;
  size_t _ackedOffset;
  std::vector<uint8_t> _image;
  Stage _stage;
  uint32_t _checksum;
  uint32_t _waitingSequence;
  uint32_t _lastSendAt;
  uint8_t _retries;

  bool sendCurrent(uint32_t now_ms);
  bool sendPacket(Packet &packet, uint32_t now_ms);
  uint32_t checksum(const std::vector<uint8_t> &image) const;
};

#endif // OTAMANAGER_H
