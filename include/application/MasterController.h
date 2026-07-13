#ifndef MASTER_CONTROLLER_H
#define MASTER_CONTROLLER_H

#include <drivers/EspNowDriver.h>
#include <network/NetworkManager.h>
#include <models/NetworkTable.h>
#include <models/Packet.h>
#include <cstdint>
#include <functional>
#include <network/PollingManager.h>
#include <network/OTAManager.h>
#include <services/DiagnosticManager.h>
#include <application/Controller.h>

class MasterController: public Controller {
public:
  using OnNodeDiscovered = std::function<void(const NodeEntry&)>;
  MasterController(IEspNowDriver &driver);
  bool begin();
  void loop();
  void setOnNodeDiscovered(OnNodeDiscovered cb) { _onNodeDiscovered = cb; }
  NetworkTable &networkTable() { return _table; }
  PollingManager &poller() { return _poller; }
  OTAManager &ota() { return _ota; }
  DiagnosticManager &diagnostics() { return _diagnostics; }
private:
  IEspNowDriver &_driver;
  NetworkManager _network;
  NetworkTable _table;
  PollingManager _poller;
  OTAManager _ota;
  DiagnosticManager _diagnostics;
  OnNodeDiscovered _onNodeDiscovered;
  uint32_t _lastPoll;
  uint32_t _lastWatchdog;
  uint32_t _watchdogInterval;
  uint16_t _nextNodeId;
  void handlePacket(const Packet &packet, const NetworkManager::MacAddress &mac);
  uint16_t nodeIdFor(const NetworkManager::MacAddress &mac);
};

#endif // MASTER_CONTROLLER_H
