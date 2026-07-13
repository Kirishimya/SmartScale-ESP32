#include <drivers/EspNowDriver.h>
#include <cstring>

#ifdef ARDUINO
#include <esp_now.h>
#include <WiFi.h>
#endif

class EspNowDriverStub : public IEspNowDriver {
public:
  EspNowDriverStub() : _cb(nullptr), _txCb(nullptr) {}
  bool begin() override { return true; }
  bool send(const uint8_t *data, size_t len, const uint8_t mac[6]) override {
    const bool ok = data && len > 0 && mac;
    if (_txCb && mac) _txCb(mac, ok);
    return ok;
  }
  bool localMac(uint8_t mac[6]) const override {
    if (!mac) return false;
    const uint8_t stubMac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
    std::memcpy(mac, stubMac, sizeof(stubMac));
    return true;
  }
  void onReceive(RxCallback cb) override { _cb = cb; }
  void onSend(TxCallback cb) override { _txCb = cb; }
private:
  RxCallback _cb;
  TxCallback _txCb;
};

#ifdef ARDUINO
class EspNowDriverEsp32 : public IEspNowDriver {
public:
  EspNowDriverEsp32() : _cb(nullptr), _txCb(nullptr) {}
  static void onDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
    if (_instance && _instance->_cb) {
      _instance->_cb(incomingData, static_cast<size_t>(len), mac_addr);
    }
  }
  static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (_instance && _instance->_txCb) {
      _instance->_txCb(mac_addr, status == ESP_NOW_SEND_SUCCESS);
    }
  }
  bool begin() override {
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) return false;
    if (esp_now_register_recv_cb(onDataRecv) != ESP_OK) return false;
    if (esp_now_register_send_cb(onDataSent) != ESP_OK) return false;
    _instance = this;
    return true;
  }
  bool send(const uint8_t *data, size_t len, const uint8_t mac[6]) override {
    if (!data || len == 0 || !mac || len > ESP_NOW_MAX_DATA_LEN) return false;
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0;
    peerInfo.ifidx = WIFI_IF_STA;
    peerInfo.encrypt = false;
    if (!esp_now_is_peer_exist(mac)) {
      if (esp_now_add_peer(&peerInfo) != ESP_OK) return false;
    }
    esp_err_t result = esp_now_send(mac, data, len);
    return result == ESP_OK;
  }
  bool localMac(uint8_t mac[6]) const override {
    if (!mac) return false;
    WiFi.macAddress(mac);
    return true;
  }
  void onReceive(RxCallback cb) override { _cb = cb; }
  void onSend(TxCallback cb) override { _txCb = cb; }
private:
  static EspNowDriverEsp32 *_instance;
  RxCallback _cb;
  TxCallback _txCb;
};
EspNowDriverEsp32* EspNowDriverEsp32::_instance = nullptr;
#endif

IEspNowDriver* createEspNowDriver() {
#ifdef ARDUINO
  return new EspNowDriverEsp32();
#else
  return new EspNowDriverStub();
#endif
}
