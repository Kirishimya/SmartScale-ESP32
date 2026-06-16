#include "Calibration.h"

Calibration::Calibration(HX711_ADC &loadCell, int eepromAddress)
    : _loadCell(loadCell), _eepromAddress(eepromAddress) {}

float Calibration::loadSavedFactor() {
#if defined(ESP8266) || defined(ESP32)
  EEPROM.begin(512);
#endif
  float calibrationValue = 0.0f;
#if defined(ESP8266) || defined(ESP32)
  EEPROM.get(_eepromAddress, calibrationValue);
#endif
  if (isnan(calibrationValue) || calibrationValue == 0.0f || calibrationValue == 0xFFFFFFFF) {
    return 1.0f;
  }
  return calibrationValue;
}

void Calibration::calibrate(Stream &port) {
  port.println("***");
  port.println("Start calibration:");
  port.println("Place the load cell on a level stable surface.");
  port.println("Remove any load applied to the load cell.");
  port.println("Send 't' from terminal to set the tare offset.");

  bool resume = false;
  while (!resume) {
    _loadCell.update();
    if (port.available() > 0) {
      char inByte = port.read();
      if (inByte == 't') {
        _loadCell.tareNoDelay();
      }
    }
    if (_loadCell.getTareStatus() == true) {
      port.println("Tare complete");
      resume = true;
    }
  }

  port.println("Now, place your known mass on the loadcell.");
  port.println("Then send the weight of this mass in kilograms (i.e. 5.5) from terminal.");

  float knownMass = 0.0f;
  resume = false;
  while (!resume) {
    _loadCell.update();
    if (port.available() > 0) {
      knownMass = port.parseFloat();
      if (knownMass != 0.0f) {
        port.print("Known mass is: ");
        port.print(knownMass);
        port.println(" kg");
        resume = true;
      }
    }
  }

  _loadCell.refreshDataSet();
  for (int i = 0; i < 20; ++i) {
    _loadCell.update();
    delay(50);
  }
  float newCalibrationValue = _loadCell.getNewCalibration(knownMass);

  if (newCalibrationValue < 0.0f) {
    port.println("Warning: calibration factor is negative.");
    port.println("This can happen if the load cell wiring is reversed or the load cell was overloaded.");
    port.println("Use a lighter known mass and verify wiring before saving.");
  }

  port.print("New calibration value has been set to: ");
  port.print(newCalibrationValue);
  port.println(", use this as calibration value (calFactor) in your project sketch.");
  port.print("Save this value to EEPROM address ");
  port.print(_eepromAddress);
  port.println("? y/n");

  resume = false;
  while (!resume) {
    if (port.available() > 0) {
      char inByte = port.read();
      if (inByte == 'y') {
        saveValue(port, newCalibrationValue);
        resume = true;
      } else if (inByte == 'n') {
        port.println("Value not saved to EEPROM");
        resume = true;
      }
    }
  }

  port.println("End calibration");
  port.println("***");
  port.println("To re-calibrate, send 'r' from terminal.");
  port.println("For manual edit of the calibration value, send 'c' from terminal.");
  port.println("***");
}

void Calibration::changeSavedCalFactor(Stream &port) {
  float oldCalibrationValue = _loadCell.getCalFactor();
  port.println("***");
  port.print("Current value is: ");
  port.println(oldCalibrationValue);
  port.println("Now, send the new value from terminal, i.e. 696.0");

  bool resume = false;
  float newCalibrationValue = 0.0f;
  while (!resume) {
    if (port.available() > 0) {
      newCalibrationValue = port.parseFloat();
      if (newCalibrationValue != 0.0f) {
        port.print("New calibration value is: ");
        port.println(newCalibrationValue);
        _loadCell.setCalFactor(newCalibrationValue);
        resume = true;
      }
    }
  }

  port.print("Save this value to EEPROM address ");
  port.print(_eepromAddress);
  port.println("? y/n");

  resume = false;
  while (!resume) {
    if (port.available() > 0) {
      char inByte = port.read();
      if (inByte == 'y') {
        saveValue(port, newCalibrationValue);
        resume = true;
      } else if (inByte == 'n') {
        port.println("Value not saved to EEPROM");
        resume = true;
      }
    }
  }

  port.println("End change calibration value");
  port.println("***");
}

float Calibration::readSavedValue() {
#if defined(ESP8266) || defined(ESP32)
  EEPROM.begin(512);
#endif
  float calibrationValue = 0.0f;
#if defined(ESP8266) || defined(ESP32)
  EEPROM.get(_eepromAddress, calibrationValue);
#endif
  return calibrationValue;
}

void Calibration::writeSavedValue(float value) {
#if defined(ESP8266) || defined(ESP32)
  EEPROM.begin(512);
  EEPROM.put(_eepromAddress, value);
  EEPROM.commit();
#endif
}

void Calibration::saveValue(Stream &port, float value) {
  writeSavedValue(value);
  port.print("Value ");
  port.print(value);
  port.print(" saved to EEPROM address: ");
  port.println(_eepromAddress);
}
