# SmartScale-ESP32

PlatformIO/Arduino firmware for a networked ESP32-C3 smart scale. The project now builds two roles: a scale/slave node connected to an HX711 load-cell amplifier, and an ESP-NOW master that discovers nodes, polls them, buffers gateway records, and coordinates OTA.

## Features
- Uses `HX711_ADC` library for HX711 integration.
- Calibration and calibration factor saved to EEPROM.
- Runtime-safe tare via serial/BLE command (`t`).
- Optional BLE output for connected clients (ESP32 BLE implementation).
- Real-time BLE JSON stream with weight and estimated piece count.
- ESP-NOW master/slave protocol with discovery, join, polling, ACK/NACK, replay filtering, and persistent retry queue.
- Portable binary payloads for sensor data, diagnostics, ACK, time sync, config, commands, and OTA metadata.
- OTA-capable partition layout with master-driven transfer and slave-side `Update` installation.
- Flash-backed gateway buffer with JSON Lines records for a Raspberry Pi/MQTT/HTTP bridge.
- Structured logger, runtime config, and watchdog services.

## Files of interest
- [src/main.cpp](src/main.cpp) — program entry and pin configuration.
- [src/application/Application.cpp](src/application/Application.cpp) — role bootstrap and runtime tasks.
- [src/application/MasterController.cpp](src/application/MasterController.cpp) — master role behavior.
- [src/application/SlaveController.cpp](src/application/SlaveController.cpp) — slave role behavior.
- [src/ScaleManager.cpp](src/ScaleManager.cpp) — core logic, printing and streams.
- [include/models/Payloads.h](include/models/Payloads.h) — portable protocol payload schemas.
- [src/services/GatewayManager.cpp](src/services/GatewayManager.cpp) — flash-backed gateway records.
- [custom_partition.csv](custom_partition.csv) — OTA-capable partition table.
- [platformio.ini](platformio.ini) — build environments: `seeed_xiao_esp32c3` and `seeed_xiao_esp32c3_master`.

Hardware / Wiring
- Default pins are set in [src/main.cpp](src/main.cpp): `HX711_dout = 6`, `HX711_sck = 5`.
- Use safe GPIO pins on ESP32-C3 (avoid flash pins 9 and 10).
- Connect load cell -> HX711 -> ESP32 as usual (DOUT -> dout pin, SCK -> sck pin, VCC, GND).

## Quick start
1. Open project in PlatformIO/VSCode.
2. Build a slave/scale firmware:

```bash
pio run -e seeed_xiao_esp32c3
```

3. Build a master firmware:

```bash
pio run -e seeed_xiao_esp32c3_master
```

4. Upload the desired role:

```bash
pio run -e seeed_xiao_esp32c3 -t upload
pio run -e seeed_xiao_esp32c3_master -t upload
```

5. Open serial monitor (115200 baud):

```bash
pio device monitor --environment seeed_xiao_esp32c3 --baud 115200
```

Because the partition table now includes OTA slots, erase/flash fully when migrating a board from the old single-app layout.

## Calibration & Tare
- To tare (set zero): send `t` from the serial monitor. The code uses non-blocking tare so it prints "Tare complete" when finished.
- Interactive calibration/batch commands are intentionally disabled during runtime tasks so they cannot block ESP-NOW/watchdog. Production calibration should be exposed through a bounded command/config flow.
- Calibration factor is saved at EEPROM address `0` by default (see [include/Calibration.h](include/Calibration.h)).

## Gateway
The master stores gateway-bound records as JSON Lines in a persistent queue. A Raspberry Pi bridge can drain `GatewayManager::popRecord()` and forward records via MQTT or HTTP. See [docs/architecture.md](docs/architecture.md) for an example record.

BLE
- BLE is enabled on ESP32 builds. The BLE device name is set in [include/BLEStream.h](include/BLEStream.h) (`ESP32C3_Scale` by default).
- BLE output sends a JSON object on each update with fields `weight`, `estimated_parts`, `avg_piece_weight`, `tare`, and `millis`.

Troubleshooting
- If weight reads a large non-zero value after reboot: the calibration factor may be loaded but the tare offset is not persisted. Either tare after power-up (send `t`), or consider enabling persistent tare in code.
- If HX711 doesn't respond, check DOUT/SCK wiring and power to the load cell.
- If calibration factor is negative, verify load cell wiring polarity and that the known mass used for calibration was appropriate.

## Testing
- Run the host-side test suite with:

```bash
bash test/run_tests.sh
```

- The suite covers serializer hardening, payload schemas, replay helpers, queue retry, network table liveness, gateway records, and state-machine smoke flow. Firmware builds are validated with both PlatformIO environments.

Contributing
- Improvements, translations, and documentation fixes are welcome. Open an issue or PR.

License
- MIT
