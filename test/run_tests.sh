#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

mkdir -p build/tests

g++ -std=c++17 -Iinclude -I. test/test_serializer/test_serializer.cpp src/utils/Serializer.cpp -o build/tests/test_serializer
./build/tests/test_serializer

cat > build/tests/test_transport.cpp <<'EOF'
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
EOF

g++ -std=c++17 -Iinclude -I. build/tests/test_transport.cpp src/utils/Serializer.cpp -o build/tests/test_transport
./build/tests/test_transport

g++ -std=c++17 -Iinclude -I. test/test_states/test_states.cpp src/slave/StateFactory.cpp src/slave/StateMachine.cpp src/slave/States.cpp -o build/tests/test_states
./build/tests/test_states

g++ -std=c++17 -Iinclude -I. test/test_flash_queue/test_flash_queue.cpp src/slave/FlashQueue.cpp -o build/tests/test_flash_queue
./build/tests/test_flash_queue

echo "All tests passed"
