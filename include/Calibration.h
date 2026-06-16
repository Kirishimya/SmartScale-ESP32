#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <Arduino.h>
#include <HX711_ADC.h>
#if defined(ESP8266) || defined(ESP32)
#include <EEPROM.h>
#endif

class Calibration {
public:
  Calibration(HX711_ADC &loadCell, int eepromAddress);
  float loadSavedFactor();
  void calibrate(Stream &port);
  void changeSavedCalFactor(Stream &port);

private:
  float readSavedValue();
  void writeSavedValue(float value);
  void saveValue(Stream &port, float value);

  HX711_ADC &_loadCell;
  int _eepromAddress;
};

#endif // CALIBRATION_H
