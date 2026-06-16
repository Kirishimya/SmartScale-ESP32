#ifndef SCALEMANAGER_H
#define SCALEMANAGER_H

#include <Arduino.h>
#include <HX711_ADC.h>

class ScaleManager {
public:
  ScaleManager(int dout, int sck, int eepromAddress);
  void begin();
  void update();
  void process();

private:
  void initializeBluetooth();
  void printWeight();
  void handleStream(Stream &port);
  void handleCommands(Stream &port);
  void printTareStatus();

  HX711_ADC _loadCell;
  class Calibration *_calibration;
  Stream *_bluetoothStream;
  unsigned long _lastPrint;
  unsigned long _lastNoDataPrint;
  bool _newDataReady;
  int _eepromAddress;
};

#endif // SCALEMANAGER_H
