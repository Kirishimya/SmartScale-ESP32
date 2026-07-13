#ifndef ESPNOW_DRIVER_H
#define ESPNOW_DRIVER_H

#include <cstdint>
#include <cstddef>
#include <functional>

class IEspNowDriver {
public:
  using RxCallback = std::function<void(const uint8_t *data, size_t len, const uint8_t *mac)>;
  using TxCallback = std::function<void(const uint8_t *mac, bool success)>;
  virtual ~IEspNowDriver() = default;
  virtual bool begin() = 0;
  virtual bool send(const uint8_t *data, size_t len, const uint8_t mac[6]) = 0;
  virtual bool localMac(uint8_t mac[6]) const = 0;
  virtual void onReceive(RxCallback cb) = 0;
  virtual void onSend(TxCallback cb) = 0;
};

// Create a platform-specific ESP-NOW driver.
IEspNowDriver* createEspNowDriver();

#endif // ESPNOW_DRIVER_H
