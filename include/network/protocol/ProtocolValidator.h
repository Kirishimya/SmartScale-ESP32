#ifndef PROTOCOL_VALIDATOR_H
#define PROTOCOL_VALIDATOR_H

#include <models/Packet.h>
#include <optional>

struct ProtocolValidator {
  static bool isValidPacket(const Packet &pkt);
  static bool isValidSequence(uint32_t seq, uint32_t lastSeq);
  static bool isValidTimestamp(uint64_t ts);
  static bool isValidAddress(const std::array<uint8_t, 6> &mac);
};

#endif // PROTOCOL_VALIDATOR_H
