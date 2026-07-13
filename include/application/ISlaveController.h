#ifndef ISLAVECONTROLLER_H
#define ISLAVECONTROLLER_H

#include <models/Packet.h>
#include <cstdint>
#include <string>

class StateMachine;

class ISlaveController {
public:
  virtual ~ISlaveController() {}
  // Time helpers
  virtual uint64_t nowMillis() = 0;
  // Networking
  virtual void sendPacket(const Packet &p) = 0;
  virtual void broadcastDiscovery() = 0;
  virtual void sendSensorSample() = 0;
  // Persistence
  virtual void persistNodeId(uint16_t nodeId) = 0;
  // Logging
  virtual void log(const std::string &msg) = 0;
  // Called by states to request a transition (StateMachine owned by controller)
  virtual void requestStateTransition(const std::string &stateName) = 0;
  // Convenience: allow states to directly request a state object by name
  // The controller implementation may map these names to concrete states.
};

#endif // ISLAVECONTROLLER_H
