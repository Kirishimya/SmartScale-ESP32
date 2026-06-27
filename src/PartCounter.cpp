#include "PartCounter.h"
#include <math.h>

PartCounter::PartCounter(HX711_ADC &loadCell, int eepromAddress)
    : _loadCell(loadCell), _eepromAddress(eepromAddress), _avgPieceWeight(0.0f), _batchActive(false), _batchStartTime(0) {}

void PartCounter::loadSavedPieceWeight() {
#if defined(ESP8266) || defined(ESP32)
  EEPROM.begin(512);
#endif
  float saved = 0.0f;
#if defined(ESP8266) || defined(ESP32)
  EEPROM.get(_eepromAddress + sizeof(float), saved);
#endif
  if (isnan(saved) || saved <= 0.0f) {
    _avgPieceWeight = 0.0f;
  } else {
    _avgPieceWeight = saved;
  }
}

void PartCounter::savePieceWeight(float w) {
#if defined(ESP8266) || defined(ESP32)
  EEPROM.begin(512);
  EEPROM.put(_eepromAddress + sizeof(float), w);
  EEPROM.commit();
#endif
  _avgPieceWeight = w;
}

void PartCounter::startSample(Stream &port) {
  port.println("Starting sampling: place a known number of parts on the scale, then send the count (integer).");
  // wait for count
  bool resume = false;
  while (!resume) {
    _loadCell.update();
    if (port.available() > 0) {
      int count = port.parseInt();
      if (count > 0) {
        float net = _loadCell.getData();
        float piece = net / (float)count;
        port.print("Measured total (kg): ");
        port.println(net, 4);
        port.print("Computed piece weight (kg): ");
        port.println(piece, 6);
        port.print("Save this as average piece weight? y/n\n");
        // wait for y/n
        bool done = false;
        while (!done) {
          if (port.available() > 0) {
            char in = port.read();
            if (in == 'y') {
              savePieceWeight(piece);
              port.println("Saved average piece weight to EEPROM.");
              done = true;
            } else if (in == 'n') {
              port.println("Not saved.");
              done = true;
            }
          }
          _loadCell.update();
        }
        resume = true;
      }
    }
  }
}

void PartCounter::startBatch(Stream &port) {
  port.println("Batch start: taring and waiting for parts...");
  _loadCell.tareNoDelay();
  _batchActive = true;
  _batchStartTime = millis();
}

void PartCounter::endBatch(Stream &port) {
  if (!_batchActive) {
    port.println("No active batch.");
    return;
  }
  // ensure we have updated data
  _loadCell.refreshDataSet();
  for (int i = 0; i < 10; ++i) {
    _loadCell.update();
    delay(20);
  }
  float net = _loadCell.getData();
  port.print("Batch net weight (kg): ");
  port.println(net, 4);
  if (_avgPieceWeight > 0.0f) {
    int estimated = (int)round(net / _avgPieceWeight);
    port.print("Estimated parts: ");
    port.println(estimated);
  } else {
    port.println("Average piece weight not set. Use 'm' to sample and save.");
  }
  _batchActive = false;
}

int PartCounter::estimateParts(float netWeight) const {
  if (_avgPieceWeight <= 0.0f) {
    return 0;
  }
  return (int)round(netWeight / _avgPieceWeight);
}

bool PartCounter::hasAvgPieceWeight() const { return _avgPieceWeight > 0.0f; }

float PartCounter::getAvgPieceWeight() const { return _avgPieceWeight; }

bool PartCounter::isBatchActive() const { return _batchActive; }
