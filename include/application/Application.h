#ifndef APPLICATION_H
#define APPLICATION_H

#include <cstdint>
#include <memory>

#include <services/RuntimeConfig.h>
#include <services/WatchdogManager.h>

class Controller;
class IEspNowDriver;
class ScaleManager;

class Application {
public:
  static Application &instance();
  ~Application();

  bool begin();
  void loop();

  void init();
  void run();

private:
  Application() = default;

  bool startRuntimeTasks();
  void controlLoopOnce();
  void sensorLoopOnce();
  void watchdogLoopOnce();
  float readScaleWeight();

#ifdef ARDUINO
  static void controlTaskThunk(void *arg);
  static void sensorTaskThunk(void *arg);
  static void watchdogTaskThunk(void *arg);
#endif

  RuntimeConfig _config;
  WatchdogManager _watchdog;
  std::unique_ptr<IEspNowDriver> _driver;
  std::unique_ptr<Controller> _controller;
  std::unique_ptr<ScaleManager> _scale;
  bool _started = false;
  bool _tasksStarted = false;
};

#endif // APPLICATION_H
