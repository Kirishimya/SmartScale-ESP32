#ifndef SLAVECONTROLLER_H
#define SLAVECONTROLLER_H

#include <drivers/EspNowDriver.h>
#include <models/Packet.h>
#include <application/ISlaveController.h>
#include <application/slave/StateMachine.h>
#include <storage/FlashQueue.h>
#include <services/SensorManager.h>
#include <network/NetworkManager.h>
#include <application/Controller.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

class SlaveController : public ISlaveController, public Controller {
public:
  SlaveController(IEspNowDriver &driver);
  bool begin() override;
  void loop() override;

  // ISlaveController implementations
  uint64_t nowMillis() override;
  void sendPacket(const Packet &p) override;
  void broadcastDiscovery() override;
  void sendSensorSample() override;
  void persistNodeId(uint16_t nodeId) override;
  void log(const std::string &msg) override;
  void requestStateTransition(const std::string &stateName) override;
  void sendSensorData(float weight, float weightPerUnit, float estimatedUnits, uint32_t seq, uint64_t ts);
  void setWeightReader(SensorManager::WeightReader reader) { _sensors.setReader(std::move(reader)); }

private:
  NetworkManager _network;
  StateMachine _sm;
  FlashQueue _outQueue;
  SensorManager _sensors;
  uint32_t _lastHeartbeat;
  uint32_t _lastSequence;
  uint32_t _heartbeatInterval;
  uint16_t _nodeId;
  uint64_t _lastUpdate;
  int64_t _timeOffsetMs = 0;
  std::optional<NetworkManager::MacAddress> _masterMac;
  uint16_t _masterId;
  struct OtaSession {
    bool active = false;
    bool installReady = false;
    uint32_t expectedSize = 0;
    uint32_t expectedChecksum = 0;
    uint32_t received = 0;
    uint32_t checksum = 2166136261UL;
  } _ota;
  void handlePacket(const Packet &packet, const NetworkManager::MacAddress &mac);
  std::optional<NetworkManager::MacAddress> destinationFor(const Packet &packet) const;
  void handleCommand(const Packet &packet);
  void handleConfig(const Packet &packet);
  void handleOta(const Packet &packet);
  bool beginOtaInstall(uint32_t size);
  bool writeOtaChunk(const std::vector<uint8_t> &chunk);
  bool endOtaInstall();
  void abortOtaInstall();
  void sendAck(uint32_t sequence);
  void sendNack(uint32_t sequence, const std::string &reason);
};

#endif // SLAVECONTROLLER_H
