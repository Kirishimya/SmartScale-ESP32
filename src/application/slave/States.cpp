#include <application/slave/States.h>
#include <application/ISlaveController.h>
#include <application/slave/StateMachine.h>
#include <models/Packet.h>
#include <iostream>

namespace {

constexpr uint32_t kBootDelayMs = 500;
constexpr uint32_t kDiscoveryBroadcastMs = 1000;
constexpr uint32_t kJoinTimeoutMs = 10000;
constexpr uint32_t kPollTimeoutMs = 4000;
constexpr uint32_t kAckTimeoutMs = 3000;
constexpr uint32_t kRecoveryTimeoutMs = 5000;

} // namespace

BootState::BootState(StateMachine &sm, ISlaveController &ctrl) : State(sm, ctrl) {}
void BootState::onEnter() { _ctrl.log("Entering BOOT"); }
void BootState::onExit() { _ctrl.log("Exiting BOOT"); }
void BootState::update(uint32_t dt_ms) {
  _elapsed += dt_ms;
  if (_elapsed >= kBootDelayMs) {
    _sm.transitionTo(std::make_unique<InitState>(_sm, _ctrl));
  }
}
const char *BootState::name() const { return "BOOT"; }

InitState::InitState(StateMachine &sm, ISlaveController &ctrl) : State(sm, ctrl) {}
void InitState::onEnter() { _ctrl.log("Entering INIT"); }
void InitState::onExit() { _ctrl.log("Exiting INIT"); }
void InitState::update(uint32_t) {
  _sm.transitionTo(std::make_unique<DiscoveryState>(_sm, _ctrl));
}
const char *InitState::name() const { return "INIT"; }

DiscoveryState::DiscoveryState(StateMachine &sm, ISlaveController &ctrl) : State(sm, ctrl) {}
void DiscoveryState::onEnter() {
  _ctrl.log("Entering DISCOVERY");
  _ctrl.broadcastDiscovery();
}
void DiscoveryState::onExit() { _ctrl.log("Exiting DISCOVERY"); }
void DiscoveryState::update(uint32_t) {
  _sm.transitionTo(std::make_unique<WaitJoinState>(_sm, _ctrl));
}
const char *DiscoveryState::name() const { return "DISCOVERY"; }

WaitJoinState::WaitJoinState(StateMachine &sm, ISlaveController &ctrl) : State(sm, ctrl) {}
void WaitJoinState::onEnter() { _ctrl.log("Entering WAIT_JOIN"); }
void WaitJoinState::onExit() { _ctrl.log("Exiting WAIT_JOIN"); }
void WaitJoinState::update(uint32_t dt_ms) {
  _elapsed += dt_ms;
  if (_elapsed >= kJoinTimeoutMs) {
    _sm.transitionTo(std::make_unique<DiscoveryState>(_sm, _ctrl));
  }
}
const char *WaitJoinState::name() const { return "WAIT_JOIN"; }

TimeSyncState::TimeSyncState(StateMachine &sm, ISlaveController &ctrl) : State(sm, ctrl) {}
void TimeSyncState::onEnter() { _ctrl.log("Entering TIME_SYNC"); }
void TimeSyncState::onExit() { _ctrl.log("Exiting TIME_SYNC"); }
void TimeSyncState::update(uint32_t dt_ms) {
  _elapsed += dt_ms;
  if (_elapsed >= kJoinTimeoutMs) {
    _sm.transitionTo(std::make_unique<RecoveryState>(_sm, _ctrl));
  }
}
const char *TimeSyncState::name() const { return "TIME_SYNC"; }

ReadyState::ReadyState(StateMachine &sm, ISlaveController &ctrl) : State(sm, ctrl) {}
void ReadyState::onEnter() { _ctrl.log("Entering READY"); }
void ReadyState::onExit() { _ctrl.log("Exiting READY"); }
void ReadyState::update(uint32_t) {}
const char *ReadyState::name() const { return "READY"; }

WaitPollState::WaitPollState(StateMachine &sm, ISlaveController &ctrl) : State(sm, ctrl) {}
void WaitPollState::onEnter() { _ctrl.log("Entering WAIT_POLL"); }
void WaitPollState::onExit() { _ctrl.log("Exiting WAIT_POLL"); }
void WaitPollState::update(uint32_t dt_ms) {
  _elapsed += dt_ms;
  if (_elapsed >= kPollTimeoutMs) {
    _sm.transitionTo(std::make_unique<SendDataState>(_sm, _ctrl));
  }
}
const char *WaitPollState::name() const { return "WAIT_POLL"; }

SendDataState::SendDataState(StateMachine &sm, ISlaveController &ctrl) : State(sm, ctrl) {}
void SendDataState::onEnter() { _ctrl.log("Entering SEND_DATA"); }
void SendDataState::onExit() { _ctrl.log("Exiting SEND_DATA"); }
void SendDataState::update(uint32_t) {
  _ctrl.sendSensorSample();
  _sm.transitionTo(std::make_unique<WaitAckState>(_sm, _ctrl));
}
const char *SendDataState::name() const { return "SEND_DATA"; }

WaitAckState::WaitAckState(StateMachine &sm, ISlaveController &ctrl) : State(sm, ctrl) {}
void WaitAckState::onEnter() { _ctrl.log("Entering WAIT_ACK"); }
void WaitAckState::onExit() { _ctrl.log("Exiting WAIT_ACK"); }
void WaitAckState::update(uint32_t dt_ms) {
  _elapsed += dt_ms;
  if (_elapsed >= kAckTimeoutMs) {
    _sm.transitionTo(std::make_unique<RecoveryState>(_sm, _ctrl));
  }
}
const char *WaitAckState::name() const { return "WAIT_ACK"; }

IdleState::IdleState(StateMachine &sm, ISlaveController &ctrl) : State(sm, ctrl) {}
void IdleState::onEnter() { _ctrl.log("Entering IDLE"); }
void IdleState::onExit() { _ctrl.log("Exiting IDLE"); }
void IdleState::update(uint32_t) {}
const char *IdleState::name() const { return "IDLE"; }

SleepState::SleepState(StateMachine &sm, ISlaveController &ctrl) : State(sm, ctrl) {}
void SleepState::onEnter() { _ctrl.log("Entering SLEEP"); }
void SleepState::onExit() { _ctrl.log("Exiting SLEEP"); }
void SleepState::update(uint32_t) {}
const char *SleepState::name() const { return "SLEEP"; }

RecoveryState::RecoveryState(StateMachine &sm, ISlaveController &ctrl) : State(sm, ctrl) {}
void RecoveryState::onEnter() { _ctrl.log("Entering RECOVERY"); }
void RecoveryState::onExit() { _ctrl.log("Exiting RECOVERY"); }
void RecoveryState::update(uint32_t dt_ms) {
  _elapsed += dt_ms;
  if (_elapsed >= kRecoveryTimeoutMs) {
    _sm.transitionTo(std::make_unique<DiscoveryState>(_sm, _ctrl));
  }
}
const char *RecoveryState::name() const { return "RECOVERY"; }
