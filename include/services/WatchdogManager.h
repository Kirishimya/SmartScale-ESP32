#ifndef WATCHDOG_MANAGER_H
#define WATCHDOG_MANAGER_H

#include <array>
#include <cstddef>
#include <cstdint>

enum class RuntimeTask : uint8_t {
  Network = 0,
  Sensor = 1,
  Storage = 2,
  Diagnostics = 3,
  Watchdog = 4,
  Count = 5
};

class WatchdogManager {
public:
  bool begin(uint32_t defaultTimeoutMs, bool restartOnFailure);
  bool attachCurrentTask();
  void registerTask(RuntimeTask task, const char *name, uint32_t timeoutMs = 0);
  void heartbeat(RuntimeTask task, uint32_t nowMs = 0);
  bool healthy(uint32_t nowMs = 0) const;
  void update(uint32_t nowMs = 0);

private:
  struct Slot {
    const char *name = nullptr;
    uint32_t timeoutMs = 0;
    uint32_t lastHeartbeatMs = 0;
    bool registered = false;
    bool failed = false;
  };

  uint32_t nowMillis() const;
  void handleFailure(const Slot &slot);

  std::array<Slot, static_cast<size_t>(RuntimeTask::Count)> _slots{};
  uint32_t _defaultTimeoutMs = 8000;
  bool _restartOnFailure = true;
  bool _hardwareEnabled = false;
  bool _currentTaskAttached = false;
};

#endif // WATCHDOG_MANAGER_H
