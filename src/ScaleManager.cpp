#include "ScaleManager.h"
#include "Calibration.h"
#include "BLEStream.h"
#include "PartCounter.h"
#include <math.h>
#if defined(ESP8266) || defined(ESP32)
#include <EEPROM.h>
#endif

constexpr unsigned long kSerialPrintInterval = 1000;
constexpr float kAutoZeroThreshold = 0.01f;
constexpr unsigned long kAutoZeroStableTime = 1500;
constexpr unsigned long kAutoZeroCooldown = 5000;

ScaleManager::ScaleManager(int dout, int sck, int eepromAddress)
    : _loadCell(dout, sck), _bluetoothStream(nullptr), _lastPrint(0),
      _lastNoDataPrint(0), _newDataReady(false), _eepromAddress(eepromAddress),
      _lastAutoZero(0), _autoZeroStableStart(0), _autoZeroPending(false) {
  _calibration = new Calibration(_loadCell, _eepromAddress);
  _partCounter = new PartCounter(_loadCell, _eepromAddress);
}

void ScaleManager::begin() {
  Serial.begin(115200);
  _partCounter->loadSavedPieceWeight();
  delay(10);

#if defined(ESP32)
  initializeBluetooth();
#endif

  Serial.println();
  Serial.println("Starting...");

  _loadCell.begin();
  _loadCell.setSamplesInUse(32);
  const unsigned long stabilizingTime = 2000;
  const bool tareAfterStart = true;
  _loadCell.start(stabilizingTime, tareAfterStart);

  while (_loadCell.getTareTimeoutFlag() || _loadCell.getSignalTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    delay(2000);
    _loadCell.start(stabilizingTime, tareAfterStart);
  }

  float calValue = _calibration->loadSavedFactor();
  if (calValue == 1.0f) {
    Serial.println("No saved calibration found. Using default factor (1.0).");
    Serial.println("Please send 'r' to calibrate.");
  } else {
    Serial.print("Loaded calibration value from EEPROM: ");
    Serial.println(calValue);
  }

  _loadCell.setCalFactor(calValue);
  if (calValue < 0.0f) {
    Serial.println("WARNING: loaded calibration factor is negative.");
    Serial.println("Check load cell wiring, known mass range, and tare procedure.");
  }
  Serial.println("Startup is complete");

  const unsigned long startTimeout = 5000;
  unsigned long startTime = millis();
  Serial.println("Waiting for HX711 data...");
  while (!_loadCell.update()) {
    if (millis() - startTime > startTimeout) {
      Serial.println("ERROR: HX711 did not respond in time.");
      Serial.println("Check DOUT/SCK wiring, power, and load cell connections.");
      break;
    }
  }
}

void ScaleManager::initializeBluetooth() {
#if defined(ESP32) && (defined(ESP32C3) || defined(ESP32_C3) || defined(CONFIG_IDF_TARGET_ESP32C3))
  static BLEStream ble;
  ble.begin(kBleDeviceName);
  _bluetoothStream = &ble;
  Serial.println("BLE iniciado. Pronto para parear.");
#elif defined(ESP32)
  static BluetoothSerial bluetooth;
  if (bluetooth.begin("ESP32_Scale")) {
    Serial.println("Bluetooth iniciado. Pronto para parear.");
    _bluetoothStream = &bluetooth;
  }
#endif
}

void ScaleManager::update() {
  if (_loadCell.update()) {
    _newDataReady = true;
  }
}

void ScaleManager::process() {
  if (_newDataReady) {
    maybeAutoZero();
    if (millis() > _lastPrint + kSerialPrintInterval) {
      printWeight();
      _newDataReady = false;
      _lastPrint = millis();
      _lastNoDataPrint = millis();
    }
  }

  if (!_newDataReady && millis() > _lastNoDataPrint + 5000) {
    if (_loadCell.getSignalTimeoutFlag()) {
      Serial.println("No HX711 data: signal timeout detected. Check wiring and power.");
    } else {
      Serial.println("No HX711 data yet. Waiting for sensor to become ready...");
    }
    _lastNoDataPrint = millis();
  }

  handleStream(Serial);
  if (_bluetoothStream && _bluetoothStream->available() > 0) {
    handleStream(*_bluetoothStream);
  }

  if (_loadCell.getTareStatus() == true) {
    printTareStatus();
  }
}

void ScaleManager::maybeAutoZero() {
  if (_loadCell.getTareStatus()) {
    _autoZeroPending = false;
    return;
  }

  const float weight = _loadCell.getData();
  const bool nearZero = fabsf(weight) < kAutoZeroThreshold;

  if (nearZero) {
    if (!_autoZeroPending) {
      _autoZeroPending = true;
      _autoZeroStableStart = millis();
    } else if (millis() - _autoZeroStableStart >= kAutoZeroStableTime &&
               millis() - _lastAutoZero >= kAutoZeroCooldown) {
      _loadCell.tareNoDelay();
      _lastAutoZero = millis();
      _autoZeroPending = false;
      Serial.println("Auto-zero triggered");
    }
  } else {
    _autoZeroPending = false;
  }
}

void ScaleManager::printWeight() {
  float weight = _loadCell.getData();
  float cal = _loadCell.getCalFactor();
  bool tare = _loadCell.getTareStatus();
  int estimatedParts = _partCounter->estimateParts(weight);
  bool hasPieceWeight = _partCounter->hasAvgPieceWeight();

  // Serial diagnostic output
  Serial.print("Raw: ");
  Serial.print((_loadCell.getTareOffset()));
  Serial.print(" | Weight: ");
  Serial.print(weight, 3);
  Serial.print(" kg");
  Serial.print(" | Parts: ");
  if (hasPieceWeight) {
    Serial.print(estimatedParts);
  } else {
    Serial.print("n/a");
  }
  Serial.print(" | Cal: ");
  Serial.print(cal);
  Serial.print(" | Tare: ");
  Serial.print(tare ? 1 : 0);
  Serial.print(" | millis: ");
  Serial.println(millis());

  // Bluetooth/stream JSON output (if available)
  if (_bluetoothStream) {
    _bluetoothStream->print("{\"weight\":");
    _bluetoothStream->print(weight, 3);
    _bluetoothStream->print(",\"estimated_parts\":");
    if (hasPieceWeight) {
      _bluetoothStream->print(estimatedParts);
    } else {
      _bluetoothStream->print("null");
    }
    _bluetoothStream->print(",\"avg_piece_weight\":");
    if (hasPieceWeight) {
      _bluetoothStream->print(_partCounter->getAvgPieceWeight(), 6);
    } else {
      _bluetoothStream->print("null");
    }
    _bluetoothStream->print(",\"tare\":");
    _bluetoothStream->print(tare ? "true" : "false");
    _bluetoothStream->print(",\"millis\":");
    _bluetoothStream->print(millis());
    _bluetoothStream->println("}");
  }
}

void ScaleManager::handleStream(Stream &port) {
  while (port.available() > 0) {
    char inByte = port.read();
    if (inByte == 't') {
      _loadCell.tareNoDelay();
    } else if (inByte == 'r') {
      _calibration->calibrate(port);
    } else if (inByte == 'c') {
      _calibration->changeSavedCalFactor(port);
    } else if (inByte == 'm') {
      // start interactive sampling to compute avg piece weight
      _partCounter->startSample(port);
    } else if (inByte == 'b') {
      _partCounter->startBatch(port);
    } else if (inByte == 'e') {
      _partCounter->endBatch(port);
    }
  }
}


void ScaleManager::printTareStatus() {
  Serial.println("Tare complete");
  if (_bluetoothStream) {
    _bluetoothStream->println("Tare complete");
  }
}
