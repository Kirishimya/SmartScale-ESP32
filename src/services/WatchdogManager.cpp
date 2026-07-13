#include <services/WatchdogManager.h>

#include <services/Logger.h>

#include <chrono>
#include <string>

#ifdef ARDUINO
#include <Arduino.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#endif

namespace {
size_t indexOf(RuntimeTask task) {
  return static_cast<size_t>(task);
}
}

bool WatchdogManager::begin(uint32_t defaultTimeoutMs, bool restartOnFailure) {
  _defaultTimeoutMs = defaultTimeoutMs == 0 ? 8000 : defaultTimeoutMs;
  _restartOnFailure = restartOnFailure;

#ifdef ARDUINO
  const uint32_t timeoutSeconds = (_defaultTimeoutMs + 999) / 1000;
  _hardwareEnabled = esp_task_wdt_init(timeoutSeconds == 0 ? 1 : timeoutSeconds, true) == ESP_OK;
#endif
  return true;
}

bool WatchdogManager::attachCurrentTask() {
#ifdef ARDUINO
  if (!_hardwareEnabled) return false;
  const esp_err_t result = esp_task_wdt_add(nullptr);
  _currentTaskAttached = result == ESP_OK || result == ESP_ERR_INVALID_STATE;
  return _currentTaskAttached;
#else
  _currentTaskAttached = true;
  return true;
#endif
}

void WatchdogManager::registerTask(RuntimeTask task, const char *name, uint32_t timeoutMs) {
  auto &slot = _slots[indexOf(task)];
  slot.name = name;
  slot.timeoutMs = timeoutMs == 0 ? _defaultTimeoutMs : timeoutMs;
  slot.lastHeartbeatMs = nowMillis();
  slot.registered = true;
  slot.failed = false;
}

void WatchdogManager::heartbeat(RuntimeTask task, uint32_t nowMs) {
  auto &slot = _slots[indexOf(task)];
  if (!slot.registered) return;
  slot.lastHeartbeatMs = nowMs == 0 ? nowMillis() : nowMs;
  slot.failed = false;
}

bool WatchdogManager::healthy(uint32_t nowMs) const {
  const uint32_t now = nowMs == 0 ? nowMillis() : nowMs;
  for (const auto &slot : _slots) {
    if (!slot.registered) continue;
    if (now - slot.lastHeartbeatMs > slot.timeoutMs) return false;
  }
  return true;
}

void WatchdogManager::update(uint32_t nowMs) {
  const uint32_t now = nowMs == 0 ? nowMillis() : nowMs;
  for (auto &slot : _slots) {
    if (!slot.registered || slot.failed) continue;
    if (now - slot.lastHeartbeatMs > slot.timeoutMs) {
      slot.failed = true;
      handleFailure(slot);
    }
  }

#ifdef ARDUINO
  if (_hardwareEnabled && _currentTaskAttached) {
    esp_task_wdt_reset();
  }
#endif
}

uint32_t WatchdogManager::nowMillis() const {
#ifdef ARDUINO
  return millis();
#else
  using namespace std::chrono;
  return static_cast<uint32_t>(
      duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
#endif
}

void WatchdogManager::handleFailure(const Slot &slot) {
  Logger::instance().error(
      "watchdog",
      std::string("Runtime task missed heartbeat: ") +
          (slot.name ? slot.name : "unknown"));
#ifdef ARDUINO
  if (_restartOnFailure) {
    delay(100);
    esp_restart();
  }
#else
  (void)slot;
#endif
}
