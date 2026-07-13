#ifndef BLESTREAM_H
#define BLESTREAM_H

#include <Arduino.h>

#if defined(ESP32) && (defined(ESP32C3) || defined(ESP32_C3) || defined(CONFIG_IDF_TARGET_ESP32C3))
#include <BLEDevice.h>

static const char kBleDeviceName[] = "ESP32C3_Scale";
static const char kBleServiceUUID[] = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char kBleTxUUID[] = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";
static const char kBleRxUUID[] = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";

class BLEStream;

class BLEStreamCallbacks : public BLECharacteristicCallbacks, public BLEServerCallbacks {
public:
  explicit BLEStreamCallbacks(BLEStream *stream);
  void onWrite(BLECharacteristic *pCharacteristic) override;
  void onConnect(BLEServer *pServer) override;
  void onDisconnect(BLEServer *pServer) override;

private:
  BLEStream *_stream;
};

class BLEStream : public Stream {
public:
  BLEStream();
  void begin(const char *deviceName);
  bool connected() const;
  size_t write(uint8_t c) override;
  size_t write(const uint8_t *buffer, size_t size) override;
  int available() override;
  int read() override;
  int peek() override;
  void flush() override;

  void onWriteData(const uint8_t *data, size_t len);
  void setConnected(bool connected);

private:
  static constexpr size_t kRxBufferSize = 256;
  uint8_t _rxBuffer[kRxBufferSize];
  size_t _rxHead;
  size_t _rxTail;
  bool _connected;
  BLECharacteristic *_txCharacteristic;
  BLEStreamCallbacks _callbacks;
};

#endif // ESP32 BLE

#endif // BLESTREAM_H
