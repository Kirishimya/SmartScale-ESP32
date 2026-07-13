# Architecture Review and Remaining Work

## Overall assessment
The firmware now has the P0 foundation, P1 network/protocol hardening, and P2 embedded runtime architecture implemented. It can build separate slave and master firmware images, but OTA, gateway integration, richer production diagnostics, and full integration tests are still pending.

## What is already in place
- Modular directory structure for core, network, slave, master, models, and utils.
- A packet model and serializer with CRC-based framing in [include/models/Packet.h](../include/models/Packet.h) and [src/network/protocol/Serializer.cpp](../src/network/protocol/Serializer.cpp).
- An ESP-NOW abstraction in [include/drivers/EspNowDriver.h](../include/drivers/EspNowDriver.h) and [src/drivers/EspNowDriver.cpp](../src/drivers/EspNowDriver.cpp).
- A slave-side controller and state machine in [src/application/SlaveController.cpp](../src/application/SlaveController.cpp), [src/application/slave/StateMachine.cpp](../src/application/slave/StateMachine.cpp), and [src/application/slave/States.cpp](../src/application/slave/States.cpp).
- A master-side controller with network table, polling, OTA, and diagnostics scaffolding in [src/application/MasterController.cpp](../src/application/MasterController.cpp), [src/models/NetworkTable.cpp](../src/models/NetworkTable.cpp), [src/network/PollingManager.cpp](../src/network/PollingManager.cpp), [src/network/OTAManager.cpp](../src/network/OTAManager.cpp), and [src/services/DiagnosticManager.cpp](../src/services/DiagnosticManager.cpp).
- A `NetworkManager` layer that owns packet encoding, local MAC injection, deferred RX processing, TX delivery events, and replay filtering in [src/network/NetworkManager.cpp](../src/network/NetworkManager.cpp).
- A central application runtime that owns role startup, FreeRTOS task creation, config, logging, and watchdog services in [src/application/Application.cpp](../src/application/Application.cpp).
- A working host-side test harness in [test/run_tests.sh](../test/run_tests.sh).
- A successful PlatformIO build for the ESP32-C3 target.

## Major gaps versus the prompt
- The firmware is organized around a central application orchestration layer, with role-specific startup delegated by [src/main.cpp](../src/main.cpp).
- The state machine is implemented for the basic join/time-sync/ready/send/wait-ack loop, but some industrial states still need richer behavior.
- The current slave logic has real sensor injection and a persistent TX queue, but calibration, tare, and production sampling policy still need to be completed.
- Protocol version checks, sequence checks, CRC validation, replay filtering, and radio-MAC/envelope matching are implemented; encryption/key provisioning is still pending.
- Logging, configuration, watchdog, and health management have production-oriented foundations, but still need deeper command/config surfaces.
- The architecture has a basic FreeRTOS split for control/network, sensor, and watchdog work; finer-grained OTA/gateway tasking is still pending.
- The project is not yet prepared for a Raspberry Pi gateway workflow beyond basic packet concepts.
- Doxygen documentation and full multi-node communication tests are still incomplete.

## Priority backlog
### P0 - Stability and correctness
- [x] Replace remaining placeholder behavior in the slave sensor path and queue path.
- [x] Complete the basic state transitions and state lifecycle handling for discovery, join, time sync, ready, send data, ACK wait, and recovery.
- [x] Tighten packet validation and error handling for malformed or corrupted packets.
- [ ] Add a central configuration manager for runtime settings.

### P1 - Network and protocol hardening
- [x] Add explicit sequence-number, version, and replay-protection logic.
- [x] Implement proper master-driven polling and ACK handling.
- [x] Implement a real local queue strategy with persistent buffering and resend-on-recovery behavior.
- [x] Add a proper master network table model with status, capabilities, offline tracking, and failure counters.

### P2 - Embedded runtime architecture
- [x] Introduce a proper application scheduler/orchestrator instead of direct setup/loop wiring.
- [x] Split runtime responsibilities into FreeRTOS tasks for control/network, sensor, and watchdog execution.
- [x] Implement a watchdog and recovery path for missed runtime heartbeats.
- [x] Add a structured logger with levels and production-safe toggling.

### P3 - OTA and gateway integration
- [ ] Implement a real OTA state machine (BEGIN, RECEIVE, VERIFY, INSTALL, REBOOT, REPORT).
- [ ] Add flash-backed local buffering for gateway offline scenarios.
- [ ] Define a clear gateway/Raspberry integration contract for forwarding and persistence.

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
- [ ] Evolve OTA and gateway integration.
- [ ] Expand tests and documentation coverage.
