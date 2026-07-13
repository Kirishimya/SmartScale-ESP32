#ifndef DIAGNOSTIC_MANAGER_H
#define DIAGNOSTIC_MANAGER_H

#include <models/NetworkTable.h>
#include <models/Packet.h>
#include <vector>
#include <cstdint>
#include <string>

class NetworkManager;

class DiagnosticManager {
public:
  DiagnosticManager(NetworkManager &network, NetworkTable &table);
  bool begin();
  void update(uint32_t now_ms);
  void captureNodeHealth(uint16_t nodeId, float battery, float temperature, float uptimeSec);
  bool sendDiagnostic(uint16_t nodeId, const std::string &message);
private:
  NetworkManager &_network;
  NetworkTable &_table;
  uint32_t _lastReport;
  uint32_t _intervalMs;
  struct HealthSnapshot { uint16_t nodeId; float battery; float temperature; float uptimeSec; };
  std::vector<HealthSnapshot> _snapshots;
};

#endif // DIAGNOSTIC_MANAGER_H
