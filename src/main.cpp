#include <Arduino.h>
#include "ScaleManager.h"

// Note: on ESP32-C3, pins 9 and 10 are flash pins and should not be used for HX711.
// Use safe GPIO pins such as 2 and 4 instead.
constexpr int HX711_dout = 6;
constexpr int HX711_sck = 5;
constexpr int calValEepromAddress = 0;

ScaleManager scale(HX711_dout, HX711_sck, calValEepromAddress);

void setup() {
  scale.begin();
}

void loop() {
  scale.update();
  scale.process();
}
