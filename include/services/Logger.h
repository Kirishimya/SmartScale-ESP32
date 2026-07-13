#ifndef LOGGER_H
#define LOGGER_H

#include <cstdint>
#include <string>

enum class LogLevel : uint8_t {
  Error = 0,
  Warn = 1,
  Info = 2,
  Debug = 3
};

class Logger {
public:
  static Logger &instance();

  void begin(LogLevel level = LogLevel::Info);
  void setLevel(LogLevel level);
  LogLevel level() const;

  void log(LogLevel level, const char *component, const std::string &message);
  void error(const char *component, const std::string &message);
  void warn(const char *component, const std::string &message);
  void info(const char *component, const std::string &message);
  void debug(const char *component, const std::string &message);

private:
  Logger() = default;
  LogLevel _level = LogLevel::Info;
  bool _started = false;
};

#endif // LOGGER_H
