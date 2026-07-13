#include <application/Application.h>

#include <ScaleManager.h>
#include <application/Controller.h>
#include <application/MasterController.h>
#include <application/SlaveController.h>
#include <drivers/EspNowDriver.h>
#include <services/Logger.h>

#include <string>

#ifdef ARDUINO
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#endif

#ifndef SMARTSCALE_ROLE_MASTER
#define SMARTSCALE_ROLE_MASTER 0
#endif

namespace {
#ifdef ARDUINO
SemaphoreHandle_t scaleMutex = nullptr;

bool createScaleMutex() {
  if (scaleMutex) return true;
  scaleMutex = xSemaphoreCreateMutex();
  return scaleMutex != nullptr;
}

class ScaleLock {
public:
  ScaleLock() : _locked(scaleMutex && xSemaphoreTake(scaleMutex, portMAX_DELAY) == pdTRUE) {}
  ~ScaleLock() {
    if (_locked) xSemaphoreGive(scaleMutex);
  }
  bool locked() const { return _locked; }

private:
  bool _locked;
};
#endif
}

Application &Application::instance() {
  static Application app;
  return app;
}

Application::~Application() = default;

bool Application::begin() {
  if (_started) return true;

  Logger::instance().begin(LogLevel::Info);
  if (!_config.begin("runtime")) {
    Logger::instance().warn("app", "Runtime config storage unavailable; using defaults.");
  }
  Logger::instance().setLevel(_config.data().logLevel);

  Logger::instance().info(
      "app",
      SMARTSCALE_ROLE_MASTER ? "Starting master runtime." : "Starting slave runtime.");

  _watchdog.begin(_config.data().watchdogTimeoutMs, _config.data().restartOnWatchdogFailure);
  _watchdog.registerTask(RuntimeTask::Network, "network/control");
  _watchdog.registerTask(RuntimeTask::Storage, "storage");
  _watchdog.registerTask(RuntimeTask::Watchdog, "watchdog");

#if SMARTSCALE_ROLE_MASTER
  _watchdog.registerTask(RuntimeTask::Diagnostics, "diagnostics");
#else
  _watchdog.registerTask(RuntimeTask::Sensor, "sensor");
#ifdef ARDUINO
  if (!createScaleMutex()) {
    Logger::instance().error("app", "Failed to create scale mutex.");
    return false;
  }
#endif
  _scale.reset(new ScaleManager(
      _config.data().hx711DoutPin,
      _config.data().hx711SckPin,
      _config.data().calibrationEepromAddress));
  _scale->begin();
#endif

  _driver.reset(createEspNowDriver());
  if (!_driver) {
    Logger::instance().error("app", "Failed to create ESP-NOW driver.");
    return false;
  }

#if SMARTSCALE_ROLE_MASTER
  _controller.reset(new MasterController(*_driver));
#else
  auto *slave = new SlaveController(*_driver);
  slave->setWeightReader([this] { return readScaleWeight(); });
  _controller.reset(slave);
#endif

  if (!_controller->begin()) {
    Logger::instance().error("app", "Controller initialization failed.");
    return false;
  }

  _started = true;
  if (!startRuntimeTasks()) {
    Logger::instance().warn("app", "FreeRTOS tasks unavailable; falling back to sequential loop.");
  }
  return true;
}

void Application::loop() {
  if (!_started && !begin()) return;

#ifdef ARDUINO
  if (_tasksStarted) {
    delay(_config.data().watchdogLoopIntervalMs);
    return;
  }
#endif

  sensorLoopOnce();
  controlLoopOnce();
  watchdogLoopOnce();
}

void Application::init() {
  begin();
}

void Application::run() {
  loop();
}

bool Application::startRuntimeTasks() {
#ifdef ARDUINO
  if (_tasksStarted) return true;

  TaskHandle_t controlHandle = nullptr;
  TaskHandle_t sensorHandle = nullptr;
  TaskHandle_t watchdogHandle = nullptr;

  const BaseType_t controlOk = xTaskCreate(
      controlTaskThunk,
      "app-control",
      8192,
      this,
      3,
      &controlHandle);

  BaseType_t sensorOk = pdTRUE;
#if !SMARTSCALE_ROLE_MASTER
  sensorOk = xTaskCreate(
      sensorTaskThunk,
      "app-sensor",
      8192,
      this,
      2,
      &sensorHandle);
#endif

  const BaseType_t watchdogOk = xTaskCreate(
      watchdogTaskThunk,
      "app-watchdog",
      3072,
      this,
      4,
      &watchdogHandle);

  _tasksStarted = controlOk == pdPASS && sensorOk == pdPASS && watchdogOk == pdPASS;
  if (!_tasksStarted) {
    if (controlHandle) vTaskDelete(controlHandle);
    if (sensorHandle) vTaskDelete(sensorHandle);
    if (watchdogHandle) vTaskDelete(watchdogHandle);
  }
  return _tasksStarted;
#else
  return false;
#endif
}

void Application::controlLoopOnce() {
  if (_controller) _controller->loop();
  _watchdog.heartbeat(RuntimeTask::Network);
  _watchdog.heartbeat(RuntimeTask::Storage);
#if SMARTSCALE_ROLE_MASTER
  _watchdog.heartbeat(RuntimeTask::Diagnostics);
#endif
}

void Application::sensorLoopOnce() {
#if !SMARTSCALE_ROLE_MASTER
  if (!_scale) return;
#ifdef ARDUINO
  ScaleLock lock;
  if (!lock.locked()) return;
#endif
  _scale->update();
  _scale->process();
  _watchdog.heartbeat(RuntimeTask::Sensor);
#endif
}

void Application::watchdogLoopOnce() {
  _watchdog.heartbeat(RuntimeTask::Watchdog);
  _watchdog.update();
}

float Application::readScaleWeight() {
#if SMARTSCALE_ROLE_MASTER
  return 0.0f;
#else
  if (!_scale) return 0.0f;
#ifdef ARDUINO
  ScaleLock lock;
  if (!lock.locked()) return 0.0f;
#endif
  return _scale->currentWeight();
#endif
}

#ifdef ARDUINO
void Application::controlTaskThunk(void *arg) {
  auto *app = static_cast<Application *>(arg);
  for (;;) {
    app->controlLoopOnce();
    vTaskDelay(pdMS_TO_TICKS(app->_config.data().controlLoopIntervalMs));
  }
}

void Application::sensorTaskThunk(void *arg) {
  auto *app = static_cast<Application *>(arg);
  for (;;) {
    app->sensorLoopOnce();
    vTaskDelay(pdMS_TO_TICKS(app->_config.data().sensorLoopIntervalMs));
  }
}

void Application::watchdogTaskThunk(void *arg) {
  auto *app = static_cast<Application *>(arg);
  app->_watchdog.attachCurrentTask();
  for (;;) {
    app->watchdogLoopOnce();
    vTaskDelay(pdMS_TO_TICKS(app->_config.data().watchdogLoopIntervalMs));
  }
}
#endif
