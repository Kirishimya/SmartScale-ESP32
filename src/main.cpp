#include <Arduino.h>
#include <application/Application.h>

void setup() {
  Application::instance().begin();
}

void loop() {
  Application::instance().loop();
}
