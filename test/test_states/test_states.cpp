#include <cassert>
#include <iostream>
#include <memory>
#include <string>

#include <slave/StateFactory.h>
#include <slave/StateMachine.h>
#include <slave/ISlaveController.h>

class FakeController : public ISlaveController {
public:
  uint64_t nowMillis() override { return 0; }
  void sendPacket(const Packet &) override {}
  void broadcastDiscovery() override {}
  void persistNodeId(uint16_t) override {}
  void log(const std::string &) override {}
  void requestStateTransition(const std::string &) override {}
};

int main() {
  FakeController ctrl;
  StateMachine sm(ctrl);
  auto boot = makeState("BOOT", sm, ctrl);
  assert(boot != nullptr);
  assert(std::string(boot->name()) == "BOOT");

  auto discovery = makeState("DISCOVERY", sm, ctrl);
  assert(discovery != nullptr);
  assert(std::string(discovery->name()) == "DISCOVERY");

  auto ready = makeState("READY", sm, ctrl);
  assert(ready != nullptr);
  assert(std::string(ready->name()) == "READY");

  std::cout << "state factory smoke test passed\n";
  return 0;
}
