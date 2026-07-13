#include <network/protocol/ProtocolValidator.h>
#include <cctype>

bool ProtocolValidator::isValidPacket(const Packet &pkt) {
  if (pkt.version != Packet::kProtocolVersion) return false;
  if (pkt.payload.size() > Packet::kMaxPayloadSize) return false;
  if (pkt.type < PacketType::DISCOVERY || pkt.type > PacketType::DIAGNOSTIC) return false;
  if (!isValidAddress(pkt.src_mac) || !isValidAddress(pkt.dst_mac)) return false;
  if (pkt.seq == 0) return false;
  if (pkt.ts == 0) return false;
  return true;
}

bool ProtocolValidator::isValidSequence(uint32_t seq, uint32_t lastSeq) {
  if (seq == 0) return false;
  if (lastSeq == 0) return true;
  const uint32_t delta = seq - lastSeq;
  return delta != 0 && delta < 0x80000000u;
}

bool ProtocolValidator::isValidTimestamp(uint64_t ts) {
  return ts > 0;
}

bool ProtocolValidator::isValidAddress(const std::array<uint8_t, 6> &mac) {
  for (uint8_t b : mac) if (b != 0) return true;
  return false;
}
