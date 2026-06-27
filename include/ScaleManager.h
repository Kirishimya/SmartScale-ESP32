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
  void maybeAutoZero();
  void printWeight();
  void handleStream(Stream &port);
  void handleCommands(Stream &port);
  void printTareStatus();

  HX711_ADC _loadCell;
  class Calibration *_calibration;
  class PartCounter *_partCounter;
  Stream *_bluetoothStream;
  unsigned long _lastPrint;
  unsigned long _lastNoDataPrint;
  bool _newDataReady;
  int _eepromAddress;
  unsigned long _lastAutoZero;
  unsigned long _autoZeroStableStart;
  bool _autoZeroPending;
};

#endif // SCALEMANAGER_H
