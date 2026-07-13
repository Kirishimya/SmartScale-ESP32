#include <iostream>
#include <vector>
#include <models/Packet.h>
#include <utils/Serializer.h>

int main() {
  Packet p;
  p.version = 1;
  p.type = PacketType::SENSOR_DATA;
  p.src_mac = {0xDE,0xAD,0xBE,0xEF,0x00,0x01};
  p.dst_mac = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  p.src_id = 42;
  p.dst_id = 1;
  p.seq = 1234;
  p.ts = 1610000000000ULL;
  p.payload = {0xAA,0xBB,0xCC};

  auto bytes = Serializer::serialize(p);
  auto parsed = Serializer::parse(bytes.data(), bytes.size());
  if (!parsed.has_value()) { std::cerr << "Parse failed (valid packet)\n"; return 1; }
  Packet q = parsed.value();
  if (q.version != p.version) { std::cerr << "version mismatch\n"; return 2; }
  if (q.src_id != p.src_id) { std::cerr << "src_id mismatch\n"; return 3; }
  if (q.seq != p.seq) { std::cerr << "seq mismatch\n"; return 4; }
  if (q.ts != p.ts) { std::cerr << "ts mismatch\n"; return 5; }
  if (q.payload.size() != p.payload.size()) { std::cerr << "payload size mismatch\n"; return 6; }

  // corrupt a byte and ensure parse fails
  std::vector<uint8_t> corrupt = bytes;
  if (!corrupt.empty()) corrupt[5] ^= 0xFF;
  auto parsed2 = Serializer::parse(corrupt.data(), corrupt.size());
  if (parsed2.has_value()) { std::cerr << "Parse should have failed for corrupted packet\n"; return 7; }

  std::cout << "All tests passed\n";
  return 0;
}
