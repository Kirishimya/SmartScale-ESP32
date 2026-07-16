#ifndef SCALEMANAGER_H
#define SCALEMANAGER_H

#include <Arduino.h>
#include <Calibration.h>
#include <HX711_ADC.h>
#include <PartCounter.h>
#include <memory>

class ScaleManager {
public:
  ScaleManager(int dout, int sck, int eepromAddress);
  void begin();
  void update();
  void process();
  float currentWeight();
  float averagePieceWeight() const;
  float estimatedParts();
  bool ready() const;

private:
  void initializeBluetooth();
  void maybeAutoZero();
  void printWeight();
  void handleStream(Stream &port);
  void handleCommands(Stream &port);
  void printTareStatus();

  HX711_ADC _loadCell;
  std::unique_ptr<Calibration> _calibration;
  std::unique_ptr<PartCounter> _partCounter;
  Stream *_bluetoothStream;
  unsigned long _lastPrint;
  unsigned long _lastNoDataPrint;
  bool _newDataReady;
  int _eepromAddress;
  unsigned long _lastAutoZero;
  unsigned long _autoZeroStableStart;
  bool _autoZeroPending;
  bool _sensorReady;
};

#endif // SCALEMANAGER_H
