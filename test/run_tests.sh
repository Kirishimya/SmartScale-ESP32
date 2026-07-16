#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

mkdir -p build/tests

g++ -std=c++17 -Wall -Wextra -Werror -Iinclude -I. \
  test/host/test_core.cpp \
  src/application/slave/StateFactory.cpp \
  src/application/slave/StateMachine.cpp \
  src/application/slave/States.cpp \
  src/models/NetworkTable.cpp \
  src/models/Payloads.cpp \
  src/network/protocol/ProtocolValidator.cpp \
  src/network/protocol/Serializer.cpp \
  src/services/GatewayManager.cpp \
  src/storage/FlashQueue.cpp \
  -o build/tests/test_core

./build/tests/test_core

echo "All tests passed"
