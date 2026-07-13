#ifndef OTAMANAGER_H
#define OTAMANAGER_H

#include <models/NetworkTable.h>
#include <models/Packet.h>
#include <vector>
#include <cstdint>

class NetworkManager;

class OTAManager {
public:
  OTAManager(NetworkManager &network, NetworkTable &table);
  bool begin();
  bool startOTA(uint16_t nodeId, const std::vector<uint8_t> &image);
  void update(uint32_t now_ms);
private:
  NetworkManager &_network;
  NetworkTable &_table;
  // simple state for ongoing OTA
  bool _active;
  uint16_t _targetNode;
  size_t _offset;
  std::vector<uint8_t> _image;
};

#endif // OTAMANAGER_H
