# SmartScale-ESP32

Compact PlatformIO/Arduino project for a digital scale using an HX711 load-cell amplifier and an ESP32 (Seeed XIAO ESP32-C3).

**Features**
- Uses `HX711_ADC` library for HX711 integration.
- Calibration and calibration factor saved to EEPROM.
- Tare and calibration via serial terminal commands (`t`, `r`, `c`).
- Optional BLE output for connected clients (ESP32 BLE implementation).
- Real-time BLE JSON stream with weight and estimated piece count.
 - Average piece weight sampling and storage to EEPROM.
 - Batch start/end commands to estimate piece counts from net weight.

**Files of interest**
- [src/main.cpp](src/main.cpp) — program entry and pin configuration.
- [src/ScaleManager.cpp](src/ScaleManager.cpp) — core logic, printing and streams.
- [src/Calibration.cpp](src/Calibration.cpp) — calibration routine and EEPROM save/load.
- [include/BLEStream.h](include/BLEStream.h) — BLE stream wrapper (ESP32 BLE only).
- `platformio.ini` — build environment: `seeed_xiao_esp32c3`.

Hardware / Wiring
- Default pins are set in [src/main.cpp](src/main.cpp): `HX711_dout = 6`, `HX711_sck = 5`.
- Use safe GPIO pins on ESP32-C3 (avoid flash pins 9 and 10).
- Connect load cell -> HX711 -> ESP32 as usual (DOUT -> dout pin, SCK -> sck pin, VCC, GND).

Quick start
1. Open project in PlatformIO/VSCode.
2. Build and upload to the board:

```bash
platformio run --environment seeed_xiao_esp32c3 --target upload
```

3. Open serial monitor (115200 baud):

```bash
platformio device monitor --environment seeed_xiao_esp32c3 --baud 115200
```

Calibration & Tare
- To tare (set zero): send `t` from the serial monitor. The code uses non-blocking tare so it prints "Tare complete" when finished.
- To run full calibration: send `r` and follow prompts (place known mass, enter mass in kg). The calibration factor is offered and can be saved to EEPROM.
- To manually change the saved calibration factor: send `c` and type the new value.
- Calibration factor is saved at EEPROM address `0` by default (see [include/Calibration.h](include/Calibration.h)).

Piece weight sampling & batch counting
- To sample average piece weight: send `m` and follow prompts. Place a known number of parts on the scale and send the count; the computed average piece weight can be saved to EEPROM.
- To start a batch: send `b` (this tares the scale and marks batch start).
- To end a batch and get an estimate: send `e`. The firmware will report net weight and estimated piece count (if average piece weight is set).
- Average piece weight is saved at EEPROM address `sizeof(float)` (immediately after the calibration float at address 0).

BLE
- BLE is enabled on ESP32 builds. The BLE device name is set in [include/BLEStream.h](include/BLEStream.h) (`ESP32C3_Scale` by default).
- BLE output sends a JSON object on each update with fields `weight`, `estimated_parts`, `avg_piece_weight`, `tare`, and `millis`.

Troubleshooting
- If weight reads a large non-zero value after reboot: the calibration factor may be loaded but the tare offset is not persisted. Either tare after power-up (send `t`), or consider enabling persistent tare in code.
- If HX711 doesn't respond, check DOUT/SCK wiring and power to the load cell.
- If calibration factor is negative, verify load cell wiring polarity and that the known mass used for calibration was appropriate.

Contributing
- Improvements, translations, and documentation fixes are welcome. Open an issue or PR.

License
- MIT
