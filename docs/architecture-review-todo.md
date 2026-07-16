# Architecture Review and Remaining Work

## Overall assessment
The firmware now has the P0 foundation, P1 network/protocol hardening, P2 embedded runtime/features, and P3 operational OTA/gateway/balanca hardening implemented. It can build separate slave and master firmware images using OTA-capable partitions. Remaining work is now mostly P4: automated tests, CI, deeper documentation, and production provisioning tools.

## What is already in place
- Modular directory structure for core, network, slave, master, models, and utils.
- A packet model and serializer with CRC-based framing in [include/models/Packet.h](../include/models/Packet.h) and [src/network/protocol/Serializer.cpp](../src/network/protocol/Serializer.cpp).
- An ESP-NOW abstraction in [include/drivers/EspNowDriver.h](../include/drivers/EspNowDriver.h) and [src/drivers/EspNowDriver.cpp](../src/drivers/EspNowDriver.cpp).
- A slave-side controller and state machine in [src/application/SlaveController.cpp](../src/application/SlaveController.cpp), [src/application/slave/StateMachine.cpp](../src/application/slave/StateMachine.cpp), and [src/application/slave/States.cpp](../src/application/slave/States.cpp).
- A master-side controller with network table, polling, OTA, and diagnostics scaffolding in [src/application/MasterController.cpp](../src/application/MasterController.cpp), [src/models/NetworkTable.cpp](../src/models/NetworkTable.cpp), [src/network/PollingManager.cpp](../src/network/PollingManager.cpp), [src/network/OTAManager.cpp](../src/network/OTAManager.cpp), and [src/services/DiagnosticManager.cpp](../src/services/DiagnosticManager.cpp).
- A `NetworkManager` layer that owns packet encoding, local MAC injection, deferred RX processing, TX delivery events, and replay filtering in [src/network/NetworkManager.cpp](../src/network/NetworkManager.cpp).
- A central application runtime that owns role startup, FreeRTOS task creation, config, logging, and watchdog services in [src/application/Application.cpp](../src/application/Application.cpp).
- Portable payload schemas for sensor data, ACKs, time sync, commands, and OTA protocol metadata in [include/models/Payloads.h](../include/models/Payloads.h).
- Explicit ESP-NOW channel/key configuration support through [include/services/RuntimeConfig.h](../include/services/RuntimeConfig.h) and [include/drivers/EspNowDriver.h](../include/drivers/EspNowDriver.h).
- Slave-side command/config/time-sync/diagnostic handling and OTA staging verification in [src/application/SlaveController.cpp](../src/application/SlaveController.cpp).
- Master-side OTA transfer state machine with ACK/NACK retry in [src/network/OTAManager.cpp](../src/network/OTAManager.cpp).
- OTA partition layout with `otadata`, `ota_0`, and `ota_1` in [custom_partition.csv](../custom_partition.csv).
- Slave-side OTA install using Arduino `Update`, checksum verification, reboot, and rollback-compatible partition activation in [src/application/SlaveController.cpp](../src/application/SlaveController.cpp).
- Flash-backed gateway buffer and JSONL gateway contract in [src/services/GatewayManager.cpp](../src/services/GatewayManager.cpp).
- Non-blocking HX711 startup failure handling, safer runtime commands, owned calibration/counting helpers, and BLE reconnection/fragmentation hardening in [src/ScaleManager.cpp](../src/ScaleManager.cpp) and [src/drivers/BLE/BLEStream.cpp](../src/drivers/BLE/BLEStream.cpp).
- A working host-side test harness in [test/run_tests.sh](../test/run_tests.sh).
- A successful PlatformIO build for the ESP32-C3 target.

## Major gaps versus the prompt
- The firmware is organized around a central application orchestration layer, with role-specific startup delegated by [src/main.cpp](../src/main.cpp).
- The state machine is implemented for the join/time-sync/ready/send/wait-ack loop, but some industrial states still need richer production behavior.
- The current slave logic has real sensor injection, portable sensor payloads, a persistent TX queue, non-blocking startup failure behavior, and safer runtime command handling. A richer production calibration UX can still be added later.
- Protocol version checks, sequence checks, CRC validation, replay filtering, radio-MAC/envelope matching, portable schemas, and optional ESP-NOW key configuration are implemented. Provisioning UI/tools for production keys are still pending.
- Logging, configuration, watchdog, command handling, and health reporting have production-oriented foundations.
- The architecture has a basic FreeRTOS split for control/network, sensor, and watchdog work; gateway records are retained in flash as JSONL for a Raspberry/gateway process to drain.
- Doxygen documentation and full multi-node communication tests are still incomplete.

## Priority backlog
### P0 - Stability and correctness
- [x] Replace remaining placeholder behavior in the slave sensor path and queue path.
- [x] Complete the basic state transitions and state lifecycle handling for discovery, join, time sync, ready, send data, ACK wait, and recovery.
- [x] Tighten packet validation and error handling for malformed or corrupted packets.
- [x] Add a central configuration manager for runtime settings.

### P1 - Network and protocol hardening
- [x] Add explicit sequence-number, version, and replay-protection logic.
- [x] Implement proper master-driven polling and ACK handling.
- [x] Implement a real local queue strategy with persistent buffering and resend-on-recovery behavior.
- [x] Add a proper master network table model with status, capabilities, offline tracking, and failure counters.
- [x] Replace ABI-dependent float payloads with portable big-endian payload schemas.
- [x] Add explicit ESP-NOW channel/key configuration hooks.

### P2 - Embedded runtime architecture
- [x] Introduce a proper application scheduler/orchestrator instead of direct setup/loop wiring.
- [x] Split runtime responsibilities into FreeRTOS tasks for control/network, sensor, and watchdog execution.
- [x] Implement a watchdog and recovery path for missed runtime heartbeats.
- [x] Add a structured logger with levels and production-safe toggling.
- [x] Add slave command/config handling for basic runtime operations.
- [x] Add master-driven time sync payload and slave-side offset tracking.
- [x] Add diagnostic response support on the slave.
- [x] Add OTA transfer protocol with BEGIN/DATA/END, offset, checksum, ACK/NACK, retry, and staging verification.

### P3 - OTA and gateway integration
- [x] Implement OTA installation into a real OTA partition with Update, validation, reboot, and rollback-compatible partition selection.
- [x] Add flash-backed local buffering for gateway offline scenarios.
- [x] Define a clear gateway/Raspberry JSONL integration contract for forwarding and persistence.
- [x] Harden HX711 startup, ownership, runtime command behavior, and BLE reconnect/fragmentation.

### P4 - Testing and documentation
- [ ] Expand host-side tests for state transitions, network-table behavior, and packet security rules.
- [ ] Add integration tests for multi-node discovery, polling, and heartbeats.
- [ ] Add Doxygen-style documentation and architecture diagrams.

## Suggested implementation order
1. Finish the slave and master runtime behavior with real state transitions.
2. Harden the packet protocol and network table logic.
3. Introduce the central runtime orchestrator and FreeRTOS task split.
4. Add OTA, watchdog, logging, and configuration management.
5. Add documentation and end-to-end multi-node test coverage.

## Added priority list
- [x] Complete the basic industrial state machine for slave nodes.
- [x] Implement real local queues with resend and recovery behavior.
- [x] Add protocol validation and security checks.
- [x] Create a central configuration manager, logger, and watchdog.
- [x] Finish P1/P2 protocol schemas, config hooks, commands, diagnostics, time sync, and OTA transfer protocol.
- [x] Evolve OTA and gateway integration.
- [ ] Expand tests and documentation coverage.
