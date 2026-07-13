#include <iostream>
#include <vector>
#include <models/Packet.h>
#include <utils/Serializer.h>

int main() {
  Packet p;
  p.version = 1;
  p.type = PacketType::HEARTBEAT;
  p.src_mac = {0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};
  p.dst_mac = {0x01,0x02,0x03,0x04,0x05,0x06};
  p.src_id = 7;
  p.dst_id = 8;
  p.seq = 99;
  p.ts = 123456789ULL;
  p.payload = {0x10, 0x20, 0x30};

  auto bytes = Serializer::serialize(p);
  auto parsed = Serializer::parse(bytes.data(), bytes.size());
  if (!parsed.has_value()) {
    std::cerr << "transport round-trip failed\n";
    return 1;
  }
  std::cout << "transport round-trip ok\n";
  return 0;
}
