#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <models/Packet.h>
#include <vector>
#include <optional>

namespace Serializer {
  std::vector<uint8_t> serialize(const Packet &p);
  std::optional<Packet> parse(const uint8_t *data, size_t len);
}

#endif // SERIALIZER_H
