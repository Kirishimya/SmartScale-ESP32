# ESP-NOW Protocol Specification (SmartScale-ESP32)

This document defines the wire format used for Master↔Slave communication over ESP-NOW.

## Packet envelope (canonical)
- All multi-byte fields are in network byte order (big-endian).
- Fields:
  - `version` (1 byte) — protocol version (start at 1)
  - `type` (1 byte) — PacketType (enum class uint8)
  - `src_mac` (6 bytes) — source MAC (48-bit)
  - `dst_mac` (6 bytes) — destination MAC (48-bit) or 0xFFFFFFFFFFFF for broadcast
  - `src_id` (2 bytes) — logical ID assigned by master (0 if none)
  - `dst_id` (2 bytes)
  - `seq` (4 bytes) — sequence number (wraps)
  - `ts` (8 bytes) — epoch milliseconds (uint64)
  - `payload_len` (2 bytes)
  - `payload` (variable)
  - `crc32` (4 bytes) — CRC32 of the entire packet up to payload (excluding crc field)

Total header size before payload: 1+1+6+6+2+2+4+8+2 = 32 bytes.

After building the envelope and CRC, the packet will be encrypted/authorized using ESP-NOW AES where applicable (driver handles keys).

## PacketType (enum class PacketType : uint8_t)
- 0x01 DISCOVERY
- 0x02 JOIN_REQUEST
- 0x03 JOIN_ACCEPT
- 0x04 JOIN_DENY
- 0x05 NODE_LIST
- 0x06 HEARTBEAT
- 0x07 SENSOR_DATA
- 0x08 COMMAND
- 0x09 ACK
- 0x0A NACK
- 0x0B ERROR
- 0x0C PING
- 0x0D PONG
- 0x0E TIME_SYNC
- 0x0F CONFIG
- 0x10 OTA_BEGIN
- 0x11 OTA_DATA
- 0x12 OTA_END
- 0x13 DIAGNOSTIC

## Validation and security
- Validate `version` first.
- Check `payload_len` <= allowed maximum (e.g., 1024 bytes).
- Compute CRC32 and compare before decrypting payload (if unencrypted header), or decrypt then validate.
- Maintain per-peer sequence windows to prevent replay (store last N sequence numbers).
- Use ESP-NOW AES keys (driver) and per-network key rotation handled by Master.

## SENSOR_DATA payload (example schema)
- Payload format is TLV or compact binary. Example JSON-like fields for readability; implementation recommended as compact binary TLV.
- Example JSON (human-readable):
  {
    "sensor_type": "hx711",
    "samples": 1,
    "weight_kg": 0.1234,
    "cal": 696.0,
    "status": 0
  }

In production, implement compact TLV: [tag(uint8), len(uint8), value(bytes)] for each field.

## ACK / Retransmit
- ACK contains the `seq` of the packet being acknowledged (payload contains seq as 4 bytes).
- Retransmit policy: Master retries up to N times with exponential backoff; on failure, store the packet in FlashQueue and move on.

## TimeSync
- Master periodically sends TIME_SYNC (type 0x0E) with `ts` = epoch ms. Slave calculates offset using request/response RTT.

## OTA packets
- OTA_BEGIN: contains metadata (firmware version, size, block_size, checksum)
- OTA_DATA: fields: block_index(uint32), data(bytes)
- OTA_END: checksum and install command

## Examples
- Discovery (broadcast): version=1, type=DISCOVERY, src_mac=<slave>, dst_mac=FF..FF, payload={capabilities,fw}
- Poll (master→slave): type=COMMAND or POLL (use COMMAND with sub-type POLL) with empty payload
- Sensor data (slave→master): type=SENSOR_DATA, payload=TLV encoded sample

## Serialization notes
- Provide serializer/ deserializer utility with unit tests.
- Keep wire format stable; bump `version` when incompatible changes occur.
