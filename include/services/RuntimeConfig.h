#ifndef RUNTIME_CONFIG_H
#define RUNTIME_CONFIG_H

#include <array>
#include <cstdint>

#include <services/Logger.h>

struct RuntimeConfigData {
  uint32_t controlLoopIntervalMs = 100;
  uint32_t sensorLoopIntervalMs = 20;
  uint32_t watchdogLoopIntervalMs = 1000;
  uint32_t watchdogTimeoutMs = 8000;
  bool restartOnWatchdogFailure = true;
  LogLevel logLevel = LogLevel::Info;
  int hx711DoutPin = 6;
  int hx711SckPin = 5;
  int calibrationEepromAddress = 0;
  uint8_t espNowChannel = 1;
  bool espNowEncrypted = false;
  std::array<uint8_t, 16> espNowPmk{};
  std::array<uint8_t, 16> espNowLmk{};
};

class RuntimeConfig {
public:
  bool begin(const char *storageNamespace = "runtime");
  const RuntimeConfigData &data() const;

private:
  RuntimeConfigData _data;
};

#endif // RUNTIME_CONFIG_H
