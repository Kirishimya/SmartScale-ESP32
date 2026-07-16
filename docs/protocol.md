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
- Check `payload_len` <= `Packet::kMaxPayloadSize` (currently 200 bytes to keep the full ESP-NOW frame under 250 bytes).
- Compute CRC32 and compare before decrypting payload (if unencrypted header), or decrypt then validate.
- Maintain per-peer sequence state to prevent replay.
- Match the radio-level sender MAC with the protocol envelope source MAC.
- Use ESP-NOW AES keys when provisioned through runtime config. Empty keys disable encryption.

## Payload schemas

All multi-byte payload fields are big-endian. Payload schema helpers live in [include/models/Payloads.h](../include/models/Payloads.h).

### SENSOR_DATA
- `schema_version` uint8, currently `1`
- `flags` uint8, currently `0`
- `weight_mg` int32
- `unit_weight_mg` int32
- `estimated_units_milli` uint32
- `sample_time_ms` uint64

### ACK / NACK
- `acked_seq` uint32
- NACK may append UTF-8 diagnostic text after the first 4 bytes.

### TIME_SYNC
- `schema_version` uint8, currently `1`
- `epoch_ms` uint64
- `monotonic_ms` uint64

### COMMAND
- `command_id` uint8
- Current commands:
  - `1` Tare
  - `2` Reboot
  - `3` SendDiagnostic
  - `4` Rediscover

### CONFIG
- `config_id` uint8
- `value` uint32
- Current configs:
  - `1` heartbeat interval in milliseconds
  - `2` master node ID

## ACK / Retransmit
- ACK contains the `seq` of the packet being acknowledged.
- Slave sensor data is retained in FlashQueue until the matching ACK arrives.
- Master OTA uses stop-and-wait with bounded retry and NACK handling.

## TimeSync
- Master sends TIME_SYNC with `epoch_ms` and `monotonic_ms`. Slave calculates a local offset and stamps outgoing sensor samples with adjusted time.

## OTA packets
- `OTA_BEGIN`: `schema_version` uint8, `image_size` uint32, `fnv1a_checksum` uint32
- `OTA_DATA`: `schema_version` uint8, `offset` uint32, `data` bytes
- `OTA_END`: `schema_version` uint8, `image_size` uint32, `fnv1a_checksum` uint32
- The slave writes OTA data through Arduino `Update`, verifies checksum, ACKs, and reboots.

## Examples
- Discovery (broadcast): version=1, type=DISCOVERY, src_mac=<slave>, dst_mac=FF..FF, payload={capabilities,fw}
- Poll (master→slave): type=PING
- Sensor data (slave→master): type=SENSOR_DATA, payload=schema above

## Serialization notes
- Provide serializer/ deserializer utility with unit tests.
- Keep wire format stable; bump `version` when incompatible changes occur.
