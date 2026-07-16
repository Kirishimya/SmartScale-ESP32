# SmartScale-ESP32 — Architecture Overview

This document contains high-level class and sequence diagrams for the restructured firmware (Master/Slave, ESP-NOW), intended as the first deliverable for the migration.

## Goals
- Show core components and responsibilities
- Provide sequence diagrams for discovery and polling flows
- Keep diagrams compact and implementation-oriented

## Core components (conceptual)
- Application (bootstrap)
- Core/Scheduler/ConfigManager/Logger
- Network: EspNowDriver, Protocol, Discovery, Polling, TimeSync, Routing
- Storage: Preferences, FlashQueue (persistent circular buffer)
- Sensors: HX711Driver, SensorManager
- Tasks: NetworkTask, SensorTask, StorageTask, HeartbeatTask, OTATask
- Models: Node, Packet, NetworkTable

## Mermaid: Component Class Diagram
```mermaid
classDiagram
    class Application {
      +init()
      +run()
    }
    class EspNowDriver {
      +init()
      +send(bytes)
      +onReceive(cb)
    }
    class Protocol {
      +serialize(Packet)
      +parse(bytes)
    }
    class NetworkTask {
      +taskLoop()
    }
    class SensorManager {
      +read()
      +calibrate()
    }
    class FlashQueue {
      +push(record)
      +pop()
    }
    Application --> NetworkTask
    Application --> SensorManager
    NetworkTask --> EspNowDriver
    NetworkTask --> Protocol
    SensorManager --> FlashQueue
    Protocol --> "1" Packet : uses
```

## Sequence: Discovery / Join
```mermaid
sequenceDiagram
    autonumber
    participant S as Slave
    participant M as Master
    S->>M: broadcast JOIN_REQUEST (MAC,fw,capabilities)
    Note right of M: Master validates request (ACL/policy)
    M-->>S: JOIN_ACCEPT (assigned logical ID, config)
    S->>S: save config, persist assigned ID
    S->>M: TIME_SYNC request/ack (or wait for master time)
    M-->>S: TIME_SYNC (epoch_ms)
    S->>S: Transition to READY
```

## Sequence: Polling / Data Flow
```mermaid
sequenceDiagram
    participant M as Master
    participant S as Slave
    M->>S: POLL_NODE (logical id)
    S->>S: collect sensor sample, enqueue
    S-->>M: SENSOR_DATA (seq, ts, payload)
    M-->>S: ACK(seq)
    M->>Gateway: forward data to Raspberry (MQTT/HTTP)
```

## Notes
- Each state implements onEnter/update/onExit (State pattern).
- Tasks communicate via RT-safe queues.
- Use persistent FlashQueue for offline resilience.

## OTA flow
- The partition table provides `otadata`, `ota_0`, and `ota_1`.
- The master uses `OTAManager` as a stop-and-wait sender: `OTA_BEGIN`, ordered `OTA_DATA` chunks with offsets, and `OTA_END`.
- Each OTA packet is acknowledged by sequence. NACKs and timeouts trigger bounded retry.
- The slave writes chunks through Arduino `Update`, verifies the FNV-1a image checksum, ends the update, ACKs, and reboots.
- Rollback support is delegated to the ESP32 OTA boot selection used by `Update`.

## Gateway contract
The master stores gateway-bound records in a flash-backed queue using JSON Lines. A Raspberry Pi gateway can drain records from `GatewayManager::popRecord()` and forward them via MQTT/HTTP without changing the ESP-NOW protocol.

Example record:
```json
{"schema":"smartscale.gateway.v1","type":"sensor_data","node_id":2,"seq":42,"received_at_ms":123456,"weight_mg":10500,"unit_weight_mg":250,"estimated_units_milli":42000,"sample_time_ms":123450}
```
