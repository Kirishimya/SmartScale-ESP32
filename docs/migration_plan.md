# ESP-NOW Migration Plan — SmartScale-ESP32

Goal: Migrate the single-node BLE-based firmware to a robust ESP-NOW Master/Slave architecture suitable for industrial LPU deployments.

High-level strategy
- Incremental migration in small, testable phases.
- Keep existing BLE features operational during transition.
- Prioritize safety: persistent data, watchdogs, and OTA rollback.

Phases & Milestones

Phase 0 — Scaffolding (done)
- Add `Packet` model, `Serializer`, `IEspNowDriver` stub
- Add architecture & protocol docs
- Milestone: repository has core interfaces and docs.

Phase 1 — Serialization & Network Layer (current)
- Implement CRC32-verified `Serializer` (done).
- Implement `EspNowDriver` wrapper (stub present; implement real driver next).
- Unit tests: serializer round-trip; corrupted-packet rejection.
- Milestone: working send/receive loop with driver stub or hardware.

Phase 2 — Slave State Machine & Controller
- Implement `ISlaveController`, `StateMachine`, core states (BOOT, DISCOVERY, WAIT_JOIN, TIME_SYNC, READY) (done).
- Implement concrete `SlaveController` wiring sensor readouts, persistence, and `EspNowDriver` callbacks.
- Tests: state transition unit tests (mock controller + fake packets).
- Milestone: slave can discover and join a master in lab tests.

Phase 3 — Master Controller & Network Table
- Implement `MasterController` to manage `NetworkTable`, discovery, polling, routing, and ACK/NACK policies (basic stub done).
- Implement node registration and join acceptance flow.
- Tests: simulated node discovery; stateful network table unit tests.
- Milestone: master accepts joins and stores node entries.

Phase 4 — Reliability & Persistence
- Implement persistent FlashQueue for outgoing packets and retransmit/backoff.
- Add node heartbeat, liveness checks, and pruning in `NetworkTable`.
- Add AES key management and replay protection scaffolding.
- Tests: queue persistence across simulated resets; replay/CRC tests.
- Milestone: reliable message delivery under intermittent link.

Phase 5 — OTA and Recovery
- Implement block-based OTA protocol over ESP-NOW (master initiates; slaves apply with rollback safety)
- Add watchdog, battery/health telemetry, and diagnostics messages.
- Tests: OTA end-to-end with image verification and rollback simulation.
- Milestone: successful OTA on test node with rollback support.

Phase 6 — Integration, Load, and Field Tests
- Stress test with N simulated slaves (start with 5 → 50 → 200 depending on hardware).
- Latency and throughput measurements; optimize polling window and airtime.
- Field test on LPUs and iterate on backoff and retransmit policies.
- Milestone: stable operation under target node count and update frequency.

Testing Strategy
- Unit tests: `Serializer`, `NetworkTable`, `StateMachine` transitions; use host-compiled tests where possible.
- Integration tests: use a small harness that runs on host to simulate nodes using the same `Serializer` and `Packet` definitions.
- Hardware-in-the-loop: automated bench with 2–10 ESP32 devices for discovery/join and OTA testing.
- CI: run host-unit tests on each PR; run integration tests in a dedicated runner or manually for hardware tests.

Acceptance Criteria
- All core messages use CRC32-verified `Serializer` and reject corrupted frames.
- Master can discover and keep a network table of nodes; slaves complete join flow and reach `READY` state.
- System recovers from intermittent link loss and re-joins without user intervention.
- OTA updates complete and can rollback on failure.

Risks & Mitigations
- ESP-NOW security reliance on shared key: keep AES key provisioning secure and enable per-master keys.
- Scale limits: measure airtime and adapt polling intervals; use time-slotted sampling to reduce collisions.
- Field diversity: start with conservative retransmit/backoff; add telemetry for tuning.

Next Concrete Tasks (short-term)
1. Implement `EspNowDriver` real wrapper around ESP-NOW API and wire into `MasterController` and `SlaveController`.
2. Implement `SlaveController` and hook `StateMachine` transitions from network events.
3. Add unit tests for `StateMachine` transitions and `NetworkTable` eviction.

Files to be created/modified in next steps
- `src/network/EspNowDriver_esp32.cpp` — ESP-NOW implementation.
- `src/slave/SlaveController.*` — concrete controller integrating sensors and persistence.
- `src/master/PollingController.*` — master polling and route manager.
- `test/*` — unit and integration tests.

Timeline (example)
- Week 1: driver implementation + serializer tests.
- Week 2: slave controller + basic discovery join tests.
- Week 3: master controller + network table + integration tests.
- Week 4: reliability features + OTA prototype.
