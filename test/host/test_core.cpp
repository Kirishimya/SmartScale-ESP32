#include <application/ISlaveController.h>
#include <application/MasterController.h>
#include <application/slave/StateFactory.h>
#include <application/slave/StateMachine.h>
#include <drivers/EspNowDriver.h>
#include <models/NetworkTable.h>
#include <models/Payloads.h>
#include <network/NetworkManager.h>
#include <network/protocol/ProtocolValidator.h>
#include <network/protocol/Serializer.h>
#include <services/GatewayManager.h>
#include <storage/FlashQueue.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << std::endl;
    std::exit(1);
  }
}

Packet validPacket(PacketType type = PacketType::SENSOR_DATA) {
  Packet p;
  p.version = Packet::kProtocolVersion;
  p.type = type;
  p.src_mac = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01};
  p.dst_mac = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  p.src_id = 2;
  p.dst_id = 1;
  p.seq = 42;
  p.ts = 123456789ULL;
  p.payload = {0x10, 0x20, 0x30};
  return p;
}

void testSerializer() {
  Packet p = validPacket();
  auto bytes = Serializer::serialize(p);
  require(!bytes.empty(), "valid packet serializes");

  auto parsed = Serializer::parse(bytes.data(), bytes.size());
  require(parsed.has_value(), "valid packet parses");
  require(parsed->src_mac == p.src_mac, "src mac round-trip");
  require(parsed->dst_mac == p.dst_mac, "dst mac round-trip");
  require(parsed->seq == p.seq, "sequence round-trip");
  require(parsed->payload == p.payload, "payload round-trip");

  auto corrupt = bytes;
  corrupt[10] ^= 0x80;
  require(!Serializer::parse(corrupt.data(), corrupt.size()).has_value(), "crc rejects corruption");

  auto trailing = bytes;
  trailing.push_back(0x00);
  require(!Serializer::parse(trailing.data(), trailing.size()).has_value(), "parser rejects trailing bytes");

  require(!Serializer::parse(nullptr, bytes.size()).has_value(), "parser rejects null pointer");

  Packet invalidType = p;
  invalidType.type = static_cast<PacketType>(0xFF);
  auto invalidBytes = Serializer::serialize(invalidType);
  require(!Serializer::parse(invalidBytes.data(), invalidBytes.size()).has_value(), "parser rejects invalid enum");

  Packet oversized = p;
  oversized.payload.assign(Packet::kMaxPayloadSize + 1, 0xAA);
  require(Serializer::serialize(oversized).empty(), "serializer rejects oversized payload");
}

void testPayloads() {
  Payloads::SensorData sample;
  sample.weightMg = -1200;
  sample.unitWeightMg = 250;
  sample.estimatedUnitsMilli = 4800;
  sample.sampleTimeMs = 5555;
  auto encoded = Payloads::encodeSensorData(sample);
  Payloads::SensorData decoded;
  require(Payloads::decodeSensorData(encoded, decoded), "sensor payload decodes");
  require(decoded.weightMg == sample.weightMg, "sensor weight round-trip");
  require(decoded.unitWeightMg == sample.unitWeightMg, "unit weight round-trip");
  require(decoded.estimatedUnitsMilli == sample.estimatedUnitsMilli, "units round-trip");
  require(decoded.sampleTimeMs == sample.sampleTimeMs, "sample time round-trip");

  Payloads::TimeSync sync{100000, 99900};
  Payloads::TimeSync decodedSync;
  require(Payloads::decodeTimeSync(Payloads::encodeTimeSync(sync), decodedSync), "time sync decodes");
  require(decodedSync.epochMs == sync.epochMs, "time sync epoch round-trip");

  uint32_t ack = 0;
  require(Payloads::decodeAck(Payloads::encodeAck(0xAABBCCDD), ack), "ack decodes");
  require(ack == 0xAABBCCDD, "ack sequence round-trip");
}

void testValidator() {
  Packet p = validPacket();
  require(ProtocolValidator::isValidPacket(p), "valid packet accepted");
  p.seq = 0;
  require(!ProtocolValidator::isValidPacket(p), "seq zero rejected");
  require(ProtocolValidator::isValidSequence(2, 1), "sequence increments");
  require(!ProtocolValidator::isValidSequence(1, 1), "duplicate sequence rejected");
  require(ProtocolValidator::isValidSequence(1, 0xFFFFFFFFu), "sequence wrap accepted");
  require(!ProtocolValidator::isValidSequence(1, 2), "old sequence rejected");
}

void testFlashQueue() {
  FlashQueue queue(2);
  require(queue.begin("host_queue"), "host queue begins");
  std::array<uint8_t, 6> dest{1, 2, 3, 4, 5, 6};
  require(queue.push({1, 2, 3}, dest, 77), "push succeeds");
  require(!queue.push(std::vector<uint8_t>(3000, 0xAA), dest, 78), "oversized queue item rejected");

  QueuedPacket out;
  require(queue.popReady(out, 1000, 1000), "first packet ready immediately");
  require(out.sequence == 77, "queued sequence retained");
  require(!queue.popReady(out, 1999, 1000), "retry waits for interval");
  require(queue.popReady(out, 2000, 1000), "retry after interval");
  require(queue.markAcked(77), "ack removes matching sequence");
  require(queue.empty(), "queue empty after ack");
}

void testNetworkTable() {
  NetworkTable table(2);
  NodeEntry node;
  node.nodeId = 2;
  node.mac = {1, 2, 3, 4, 5, 6};
  node.lastSeen = 100;
  node.active = true;
  node.capabilities = 0x01;
  require(table.addOrUpdate(node), "node added");
  require(table.findById(2).has_value(), "node found by id");
  require(table.findByMac(node.mac).has_value(), "node found by mac");
  require(table.recordFailure(2), "failure recorded");
  require(table.recordFailure(2), "second failure recorded");
  require(table.recordFailure(2), "third failure recorded");
  require(!table.findById(2)->active, "node inactive after failures");
  require(table.recordSuccess(2, 500), "success recorded");
  require(table.findById(2)->active, "node active after success");
  require(table.markInactiveSince(10000, 1000) == 1, "stale node marked inactive");
}

void testGatewayManager() {
  GatewayManager gateway(4);
  require(gateway.begin("host_gateway"), "gateway begins");
  Packet packet = validPacket(PacketType::SENSOR_DATA);
  packet.payload = Payloads::encodeSensorData({1234, 12, 34000, 777});
  require(gateway.enqueuePacket(packet, packet.src_id, 9000), "gateway enqueue succeeds");
  std::string record;
  require(gateway.popRecord(record), "gateway record pops");
  require(record.find("\"schema\":\"smartscale.gateway.v1\"") != std::string::npos, "gateway schema present");
  require(record.find("\"weight_mg\":1234") != std::string::npos, "gateway sensor field present");
}

class FakeController : public ISlaveController {
public:
  uint64_t nowMillis() override { return 0; }
  void sendPacket(const Packet &) override {}
  void broadcastDiscovery() override { ++discoveries; }
  void sendSensorSample() override { ++samples; }
  void persistNodeId(uint16_t) override {}
  void log(const std::string &) override {}
  void requestStateTransition(const std::string &) override {}

  int discoveries = 0;
  int samples = 0;
};

class FakeDriver : public IEspNowDriver {
public:
  struct SentFrame {
    std::vector<uint8_t> bytes;
    std::array<uint8_t, 6> destination{};
  };

  void configure(const EspNowConfig &config) override { this->config = config; }
  bool begin() override { return true; }
  bool send(const uint8_t *data, size_t len, const uint8_t mac[6]) override {
    require(data != nullptr, "driver send data not null");
    require(mac != nullptr, "driver send mac not null");
    SentFrame frame;
    frame.bytes.assign(data, data + len);
    std::copy(mac, mac + 6, frame.destination.begin());
    sent.push_back(frame);
    if (tx) tx(mac, true);
    return true;
  }
  bool localMac(uint8_t mac[6]) const override {
    std::copy(local.begin(), local.end(), mac);
    return true;
  }
  void onReceive(RxCallback cb) override { rx = cb; }
  void onSend(TxCallback cb) override { tx = cb; }

  void inject(const Packet &packet, const std::array<uint8_t, 6> &sender) {
    auto bytes = Serializer::serialize(packet);
    require(!bytes.empty(), "injected packet serializes");
    require(static_cast<bool>(rx), "rx callback registered");
    rx(bytes.data(), bytes.size(), sender.data());
  }

  EspNowConfig config;
  std::array<uint8_t, 6> local{0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
  std::vector<SentFrame> sent;
  RxCallback rx;
  TxCallback tx;
};

void testNetworkManagerFiltering() {
  FakeDriver driver;
  NetworkManager network(driver, 1);
  require(network.begin(), "network begins");

  std::array<uint8_t, 6> peer{0x02, 0x00, 0x00, 0x00, 0x00, 0x02};
  int received = 0;
  network.setPacketHandler([&](const Packet &, const NetworkManager::MacAddress &) {
    ++received;
  });

  Packet valid = validPacket(PacketType::HEARTBEAT);
  valid.src_mac = peer;
  valid.dst_mac = driver.local;
  valid.seq = 1;
  driver.inject(valid, peer);
  network.poll();
  require(received == 1, "valid incoming packet dispatched");

  driver.inject(valid, peer);
  network.poll();
  require(received == 1, "duplicate sequence rejected");

  Packet wrongDestination = valid;
  wrongDestination.seq = 2;
  wrongDestination.dst_mac = {9, 9, 9, 9, 9, 9};
  driver.inject(wrongDestination, peer);
  network.poll();
  require(received == 1, "wrong destination rejected");

  Packet forged = valid;
  forged.seq = 3;
  forged.src_mac = {8, 8, 8, 8, 8, 8};
  driver.inject(forged, peer);
  network.poll();
  require(received == 1, "forged envelope mac rejected");
}

void testMasterJoinIntegration() {
  FakeDriver driver;
  MasterController master(driver);
  require(master.begin(), "master begins");

  std::array<uint8_t, 6> slaveMac{0x02, 0x00, 0x00, 0x00, 0x00, 0x09};
  Packet join;
  join.version = Packet::kProtocolVersion;
  join.type = PacketType::JOIN_REQUEST;
  join.src_mac = slaveMac;
  join.dst_mac = NetworkManager::broadcastMac();
  join.src_id = 0;
  join.dst_id = 0;
  join.seq = 1;
  join.ts = 1000;
  join.payload = {0x01};

  driver.inject(join, slaveMac);
  master.loop();
  require(master.networkTable().findByMac(slaveMac).has_value(), "master registers joining node");
  require(driver.sent.size() >= 2, "master sends join accept and time sync");

  auto accepted = Serializer::parse(driver.sent[0].bytes.data(), driver.sent[0].bytes.size());
  auto timeSync = Serializer::parse(driver.sent[1].bytes.data(), driver.sent[1].bytes.size());
  require(accepted.has_value(), "join accept parses");
  require(timeSync.has_value(), "time sync parses");
  require(accepted->type == PacketType::JOIN_ACCEPT, "first response is join accept");
  require(timeSync->type == PacketType::TIME_SYNC, "second response is time sync");
  Payloads::TimeSync decoded;
  require(Payloads::decodeTimeSync(timeSync->payload, decoded), "time sync payload decodes");
}

void testStateMachine() {
  FakeController ctrl;
  StateMachine sm(ctrl);
  auto boot = makeState("BOOT", sm, ctrl);
  require(boot != nullptr, "boot state created");
  sm.transitionTo(std::move(boot));
  require(std::string(sm.current()->name()) == "BOOT", "entered boot");
  sm.update(500);
  require(std::string(sm.current()->name()) == "INIT", "boot transitions to init");
  sm.update(1);
  require(std::string(sm.current()->name()) == "DISCOVERY", "init transitions to discovery");
  require(ctrl.discoveries == 1, "discovery broadcasts once");
  sm.update(1);
  require(std::string(sm.current()->name()) == "WAIT_JOIN", "discovery transitions to wait join");
}

} // namespace

int main() {
  testSerializer();
  testPayloads();
  testValidator();
  testFlashQueue();
  testNetworkTable();
  testGatewayManager();
  testNetworkManagerFiltering();
  testMasterJoinIntegration();
  testStateMachine();
  std::cout << "All host tests passed" << std::endl;
  return 0;
}
