#ifndef POLLING_MANAGER_H
#define POLLING_MANAGER_H

#include <models/NetworkTable.h>
#include <models/Packet.h>
#include <cstdint>

class NetworkManager;

class PollingManager {
public:
  PollingManager(NetworkManager &network, NetworkTable &table);
  void begin();
  void update(uint32_t now_ms);
  bool pollNode(const NodeEntry &node);
private:
  NetworkManager &_network;
  NetworkTable &_table;
  uint32_t _lastRound;
  uint32_t _intervalMs;
};

#endif // POLLING_MANAGER_H
