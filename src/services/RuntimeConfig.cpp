#include <services/RuntimeConfig.h>

#include <algorithm>

#ifdef ARDUINO
#include <Preferences.h>
#endif

namespace {
#ifndef SMARTSCALE_CONTROL_LOOP_MS
constexpr uint32_t kDefaultControlLoopMs = 100;
#else
constexpr uint32_t kDefaultControlLoopMs = SMARTSCALE_CONTROL_LOOP_MS;
#endif

#ifndef SMARTSCALE_SENSOR_LOOP_MS
constexpr uint32_t kDefaultSensorLoopMs = 20;
#else
constexpr uint32_t kDefaultSensorLoopMs = SMARTSCALE_SENSOR_LOOP_MS;
#endif

#ifndef SMARTSCALE_WATCHDOG_LOOP_MS
constexpr uint32_t kDefaultWatchdogLoopMs = 1000;
#else
constexpr uint32_t kDefaultWatchdogLoopMs = SMARTSCALE_WATCHDOG_LOOP_MS;
#endif

#ifndef SMARTSCALE_WATCHDOG_TIMEOUT_MS
constexpr uint32_t kDefaultWatchdogTimeoutMs = 8000;
#else
constexpr uint32_t kDefaultWatchdogTimeoutMs = SMARTSCALE_WATCHDOG_TIMEOUT_MS;
#endif

#ifndef SMARTSCALE_HX711_DOUT
constexpr int kDefaultHx711Dout = 6;
#else
constexpr int kDefaultHx711Dout = SMARTSCALE_HX711_DOUT;
#endif

#ifndef SMARTSCALE_HX711_SCK
constexpr int kDefaultHx711Sck = 5;
#else
constexpr int kDefaultHx711Sck = SMARTSCALE_HX711_SCK;
#endif

#ifndef SMARTSCALE_CAL_EEPROM_ADDRESS
constexpr int kDefaultCalibrationAddress = 0;
#else
constexpr int kDefaultCalibrationAddress = SMARTSCALE_CAL_EEPROM_ADDRESS;
#endif

#ifndef SMARTSCALE_ESPNOW_CHANNEL
constexpr uint8_t kDefaultEspNowChannel = 1;
#else
constexpr uint8_t kDefaultEspNowChannel = SMARTSCALE_ESPNOW_CHANNEL;
#endif

LogLevel toLogLevel(uint8_t raw) {
  if (raw > static_cast<uint8_t>(LogLevel::Debug)) return LogLevel::Info;
  return static_cast<LogLevel>(raw);
}
}

bool RuntimeConfig::begin(const char *storageNamespace) {
  _data.controlLoopIntervalMs = kDefaultControlLoopMs;
  _data.sensorLoopIntervalMs = kDefaultSensorLoopMs;
  _data.watchdogLoopIntervalMs = kDefaultWatchdogLoopMs;
  _data.watchdogTimeoutMs = kDefaultWatchdogTimeoutMs;
  _data.hx711DoutPin = kDefaultHx711Dout;
  _data.hx711SckPin = kDefaultHx711Sck;
  _data.calibrationEepromAddress = kDefaultCalibrationAddress;
  _data.espNowChannel = kDefaultEspNowChannel;

#ifdef ARDUINO
  Preferences preferences;
  if (!preferences.begin(storageNamespace ? storageNamespace : "runtime", true)) {
    return false;
  }
  _data.controlLoopIntervalMs = preferences.getUInt("control_ms", _data.controlLoopIntervalMs);
  _data.sensorLoopIntervalMs = preferences.getUInt("sensor_ms", _data.sensorLoopIntervalMs);
  _data.watchdogLoopIntervalMs = preferences.getUInt("wd_loop_ms", _data.watchdogLoopIntervalMs);
  _data.watchdogTimeoutMs = preferences.getUInt("wd_timeout", _data.watchdogTimeoutMs);
  _data.restartOnWatchdogFailure = preferences.getBool("wd_restart", _data.restartOnWatchdogFailure);
  _data.logLevel = toLogLevel(preferences.getUChar("log_level", static_cast<uint8_t>(_data.logLevel)));
  _data.hx711DoutPin = preferences.getInt("hx_dout", _data.hx711DoutPin);
  _data.hx711SckPin = preferences.getInt("hx_sck", _data.hx711SckPin);
  _data.calibrationEepromAddress = preferences.getInt("cal_addr", _data.calibrationEepromAddress);
  _data.espNowChannel = preferences.getUChar("esp_channel", _data.espNowChannel);
  _data.espNowEncrypted = preferences.getBool("esp_encrypt", _data.espNowEncrypted);
  if (preferences.getBytesLength("esp_pmk") == _data.espNowPmk.size()) {
    preferences.getBytes("esp_pmk", _data.espNowPmk.data(), _data.espNowPmk.size());
  }
  if (preferences.getBytesLength("esp_lmk") == _data.espNowLmk.size()) {
    preferences.getBytes("esp_lmk", _data.espNowLmk.data(), _data.espNowLmk.size());
  }
  preferences.end();
#endif

  if (_data.controlLoopIntervalMs == 0) _data.controlLoopIntervalMs = kDefaultControlLoopMs;
  if (_data.sensorLoopIntervalMs == 0) _data.sensorLoopIntervalMs = kDefaultSensorLoopMs;
  if (_data.watchdogLoopIntervalMs == 0) _data.watchdogLoopIntervalMs = kDefaultWatchdogLoopMs;
  if (_data.watchdogTimeoutMs < _data.watchdogLoopIntervalMs * 2) {
    _data.watchdogTimeoutMs = _data.watchdogLoopIntervalMs * 2;
  }
  if (_data.espNowChannel == 0 || _data.espNowChannel > 14) {
    _data.espNowChannel = kDefaultEspNowChannel;
  }
  const bool pmkEmpty = std::all_of(
      _data.espNowPmk.begin(), _data.espNowPmk.end(), [](uint8_t b) { return b == 0; });
  const bool lmkEmpty = std::all_of(
      _data.espNowLmk.begin(), _data.espNowLmk.end(), [](uint8_t b) { return b == 0; });
  if (pmkEmpty || lmkEmpty) {
    _data.espNowEncrypted = false;
  }
  return true;
}

const RuntimeConfigData &RuntimeConfig::data() const {
  return _data;
}
