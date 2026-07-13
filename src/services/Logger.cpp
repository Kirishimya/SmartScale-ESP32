#include <services/Logger.h>

#include <iostream>

#ifdef ARDUINO
#include <Arduino.h>
#endif

namespace {
const char *levelName(LogLevel level) {
  switch (level) {
    case LogLevel::Error: return "ERROR";
    case LogLevel::Warn: return "WARN";
    case LogLevel::Info: return "INFO";
    case LogLevel::Debug: return "DEBUG";
    default: return "UNKNOWN";
  }
}

uint64_t logMillis() {
#ifdef ARDUINO
  return millis();
#else
  return 0;
#endif
}
}

Logger &Logger::instance() {
  static Logger logger;
  return logger;
}

void Logger::begin(LogLevel level) {
  _level = level;
  if (_started) return;
#ifdef ARDUINO
  Serial.begin(115200);
#endif
  _started = true;
}

void Logger::setLevel(LogLevel level) { _level = level; }
LogLevel Logger::level() const { return _level; }

void Logger::log(LogLevel level, const char *component, const std::string &message) {
  if (static_cast<uint8_t>(level) > static_cast<uint8_t>(_level)) return;
  if (!_started) begin(_level);

#ifdef ARDUINO
  Serial.print("[");
  Serial.print(static_cast<unsigned long>(logMillis()));
  Serial.print("][");
  Serial.print(levelName(level));
  Serial.print("][");
  Serial.print(component ? component : "app");
  Serial.print("] ");
  Serial.println(message.c_str());
#else
  std::cout << "[" << logMillis() << "][" << levelName(level) << "]["
            << (component ? component : "app") << "] " << message << std::endl;
#endif
}

void Logger::error(const char *component, const std::string &message) {
  log(LogLevel::Error, component, message);
}

void Logger::warn(const char *component, const std::string &message) {
  log(LogLevel::Warn, component, message);
}

void Logger::info(const char *component, const std::string &message) {
  log(LogLevel::Info, component, message);
}

void Logger::debug(const char *component, const std::string &message) {
  log(LogLevel::Debug, component, message);
}
