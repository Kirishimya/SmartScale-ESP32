#ifndef PARTCOUNTER_H
#define PARTCOUNTER_H

#include <Arduino.h>
#include <HX711_ADC.h>
#if defined(ESP8266) || defined(ESP32)
#include <EEPROM.h>
#endif

class PartCounter {
public:
  PartCounter(HX711_ADC &loadCell, int eepromAddress);
  void loadSavedPieceWeight();
  void savePieceWeight(float w);
  int estimateParts(float netWeight) const;
  bool hasAvgPieceWeight() const;
  void startSample(Stream &port);
  void startBatch(Stream &port);
  void endBatch(Stream &port);
  float getAvgPieceWeight() const;
  bool isBatchActive() const;

private:
  HX711_ADC &_loadCell;
  int _eepromAddress;
  float _avgPieceWeight;
  bool _batchActive;
  unsigned long _batchStartTime;
};

#endif // PARTCOUNTER_H
