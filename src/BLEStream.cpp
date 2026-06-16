#include "BLEStream.h"

#if defined(ESP32) && (defined(ESP32C3) || defined(ESP32_C3) || defined(CONFIG_IDF_TARGET_ESP32C3))
#include <BLE2902.h>

BLEStreamCallbacks::BLEStreamCallbacks(BLEStream *stream)
    : _stream(stream) {}

void BLEStreamCallbacks::onWrite(BLECharacteristic *pCharacteristic) {
  std::string value = pCharacteristic->getValue();
  if (_stream && !value.empty()) {
    _stream->onWriteData(reinterpret_cast<const uint8_t *>(value.data()), value.size());
  }
}

void BLEStreamCallbacks::onConnect(BLEServer *pServer) {
  if (_stream) {
    _stream->setConnected(true);
  }
}

void BLEStreamCallbacks::onDisconnect(BLEServer *pServer) {
  if (_stream) {
    _stream->setConnected(false);
  }
}

BLEStream::BLEStream()
    : _rxHead(0), _rxTail(0), _connected(false), _txCharacteristic(nullptr),
      _callbacks(this) {}

void BLEStream::begin(const char *deviceName) {
  Serial.println("BLE: init");
  BLEDevice::init(deviceName);
  Serial.println("BLE: create server");
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(&_callbacks);

  Serial.println("BLE: create service and characteristics");
  BLEService *service = server->createService(BLEUUID(kBleServiceUUID));
  _txCharacteristic = service->createCharacteristic(
      BLEUUID(kBleTxUUID), BLECharacteristic::PROPERTY_NOTIFY);
  // Add Client Characteristic Configuration descriptor so clients can enable notifications
  _txCharacteristic->addDescriptor(new BLE2902());
  BLECharacteristic *rxCharacteristic = service->createCharacteristic(
      BLEUUID(kBleRxUUID), BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_WRITE);
  rxCharacteristic->setCallbacks(&_callbacks);

  service->start();
  Serial.println("BLE: service started");
  delay(100);
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(BLEUUID(kBleServiceUUID));
  advertising->setScanResponse(true);
  advertising->start();
  Serial.println("BLE: advertising started");
}

bool BLEStream::connected() const {
  return _connected;
}

size_t BLEStream::write(uint8_t c) {
  return write(&c, 1);
}

size_t BLEStream::write(const uint8_t *buffer, size_t size) {
  if (!_txCharacteristic || !_connected) {
    return 0;
  }
  uint8_t *value = const_cast<uint8_t *>(buffer);
  _txCharacteristic->setValue(value, size);
  _txCharacteristic->notify();
  return size;
}

int BLEStream::available() {
  if (_rxHead >= _rxTail) {
    return _rxHead - _rxTail;
  }
  return kRxBufferSize - _rxTail + _rxHead;
}

int BLEStream::read() {
  if (available() == 0) {
    return -1;
  }
  uint8_t value = _rxBuffer[_rxTail++];
  if (_rxTail >= kRxBufferSize) {
    _rxTail = 0;
  }
  return value;
}

int BLEStream::peek() {
  if (available() == 0) {
    return -1;
  }
  return _rxBuffer[_rxTail];
}

void BLEStream::flush() {
}

void BLEStream::onWriteData(const uint8_t *data, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    size_t next = (_rxHead + 1) % kRxBufferSize;
    if (next == _rxTail) {
      break;
    }
    _rxBuffer[_rxHead] = data[i];
    _rxHead = next;
  }
}

void BLEStream::setConnected(bool connected) {
  _connected = connected;
  if (connected) {
    Serial.println("BLE connected");
  } else {
    Serial.println("BLE disconnected");
  }
}

#endif // ESP32 BLE
