#include <drivers/BLE/BLEStream.h>

#if defined(ESP32) && (defined(ESP32C3) || defined(ESP32_C3) || defined(CONFIG_IDF_TARGET_ESP32C3))
#include <BLE2902.h>

#include <algorithm>

namespace {
constexpr size_t kNotifyChunkSize = 20;
portMUX_TYPE gBleMux = portMUX_INITIALIZER_UNLOCKED;
}

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
  BLEDevice::startAdvertising();
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
  size_t sent = 0;
  while (sent < size) {
    const size_t chunk = std::min(kNotifyChunkSize, size - sent);
    uint8_t *value = const_cast<uint8_t *>(buffer + sent);
    _txCharacteristic->setValue(value, chunk);
    _txCharacteristic->notify();
    sent += chunk;
    delay(2);
  }
  return size;
}

int BLEStream::available() {
  portENTER_CRITICAL(&gBleMux);
  const size_t head = _rxHead;
  const size_t tail = _rxTail;
  portEXIT_CRITICAL(&gBleMux);
  if (head >= tail) return head - tail;
  return kRxBufferSize - tail + head;
}

int BLEStream::read() {
  portENTER_CRITICAL(&gBleMux);
  if (_rxHead == _rxTail) {
    portEXIT_CRITICAL(&gBleMux);
    return -1;
  }
  uint8_t value = _rxBuffer[_rxTail++];
  if (_rxTail >= kRxBufferSize) {
    _rxTail = 0;
  }
  portEXIT_CRITICAL(&gBleMux);
  return value;
}

int BLEStream::peek() {
  portENTER_CRITICAL(&gBleMux);
  if (_rxHead == _rxTail) {
    portEXIT_CRITICAL(&gBleMux);
    return -1;
  }
  const uint8_t value = _rxBuffer[_rxTail];
  portEXIT_CRITICAL(&gBleMux);
  return value;
}

void BLEStream::flush() {
}

void BLEStream::onWriteData(const uint8_t *data, size_t len) {
  portENTER_CRITICAL(&gBleMux);
  for (size_t i = 0; i < len; ++i) {
    size_t next = (_rxHead + 1) % kRxBufferSize;
    if (next == _rxTail) {
      break;
    }
    _rxBuffer[_rxHead] = data[i];
    _rxHead = next;
  }
  portEXIT_CRITICAL(&gBleMux);
}

void BLEStream::setConnected(bool connected) {
  portENTER_CRITICAL(&gBleMux);
  _connected = connected;
  portEXIT_CRITICAL(&gBleMux);
  if (connected) {
    Serial.println("BLE connected");
  } else {
    Serial.println("BLE disconnected");
  }
}

#endif // ESP32 BLE
