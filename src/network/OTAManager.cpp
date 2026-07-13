#include <network/OTAManager.h>
#include <network/NetworkManager.h>
#include <services/Logger.h>

#include <string>

OTAManager::OTAManager(NetworkManager &network, NetworkTable &table)
  : _network(network), _table(table), _active(false), _targetNode(0), _offset(0) {}

bool OTAManager::begin() { return true; }

bool OTAManager::startOTA(uint16_t nodeId, const std::vector<uint8_t> &image) {
  _active = true;
  _targetNode = nodeId;
  _image = image;
  _offset = 0;
  Logger::instance().info(
      "ota",
      "Starting OTA to node " + std::to_string(nodeId) +
          " image size=" + std::to_string(image.size()));
  return true;
}

void OTAManager::update(uint32_t now_ms) {
  if (!_active) return;
  auto opt = _table.findById(_targetNode);
  if (!opt.has_value()) { _active = false; return; }
  auto node = opt.value();
  // send next chunk (stub: send small payload until image exhausted)
  const size_t chunk = 64;
  if (_offset >= _image.size()) {
    Logger::instance().info("ota", "OTA complete for node " + std::to_string(_targetNode));
    _active = false; return;
  }
  size_t toSend = std::min(chunk, _image.size() - _offset);
  Packet p; p.type = PacketType::OTA_DATA; p.src_id = 0; p.dst_id = _targetNode;
  p.payload.assign(_image.begin()+_offset, _image.begin()+_offset+toSend);
  _network.send(p, node.mac);
  _offset += toSend;
}
