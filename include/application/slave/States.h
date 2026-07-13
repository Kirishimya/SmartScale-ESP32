#ifndef SLAVE_STATES_H
#define SLAVE_STATES_H

#include <application/slave/State.h>

class BootState : public State {
public:
  BootState(StateMachine &sm, ISlaveController &ctrl);
  void onEnter() override;
  void onExit() override;
  void update(uint32_t dt_ms) override;
  const char *name() const override;
private:
  uint32_t _elapsed = 0;
};

class InitState : public State {
public:
  InitState(StateMachine &sm, ISlaveController &ctrl);
  void onEnter() override;
  void onExit() override;
  void update(uint32_t dt_ms) override;
  const char *name() const override;
};

class DiscoveryState : public State {
public:
  DiscoveryState(StateMachine &sm, ISlaveController &ctrl);
  void onEnter() override;
  void onExit() override;
  void update(uint32_t dt_ms) override;
  const char *name() const override;
};

class WaitJoinState : public State {
public:
  WaitJoinState(StateMachine &sm, ISlaveController &ctrl);
  void onEnter() override;
  void onExit() override;
  void update(uint32_t dt_ms) override;
  const char *name() const override;
private:
  uint32_t _elapsed = 0;
};

class TimeSyncState : public State {
public:
  TimeSyncState(StateMachine &sm, ISlaveController &ctrl);
  void onEnter() override;
  void onExit() override;
  void update(uint32_t dt_ms) override;
  const char *name() const override;
private:
  uint32_t _elapsed = 0;
};

class ReadyState : public State {
public:
  ReadyState(StateMachine &sm, ISlaveController &ctrl);
  void onEnter() override;
  void onExit() override;
  void update(uint32_t dt_ms) override;
  const char *name() const override;
};

class WaitPollState : public State {
public:
  WaitPollState(StateMachine &sm, ISlaveController &ctrl);
  void onEnter() override;
  void onExit() override;
  void update(uint32_t dt_ms) override;
  const char *name() const override;
private:
  uint32_t _elapsed = 0;
};

class SendDataState : public State {
public:
  SendDataState(StateMachine &sm, ISlaveController &ctrl);
  void onEnter() override;
  void onExit() override;
  void update(uint32_t dt_ms) override;
  const char *name() const override;
};

class WaitAckState : public State {
public:
  WaitAckState(StateMachine &sm, ISlaveController &ctrl);
  void onEnter() override;
  void onExit() override;
  void update(uint32_t dt_ms) override;
  const char *name() const override;
private:
  uint32_t _elapsed = 0;
};

class IdleState : public State {
public:
  IdleState(StateMachine &sm, ISlaveController &ctrl);
  void onEnter() override;
  void onExit() override;
  void update(uint32_t dt_ms) override;
  const char *name() const override;
};

class SleepState : public State {
public:
  SleepState(StateMachine &sm, ISlaveController &ctrl);
  void onEnter() override;
  void onExit() override;
  void update(uint32_t dt_ms) override;
  const char *name() const override;
};

class RecoveryState : public State {
public:
  RecoveryState(StateMachine &sm, ISlaveController &ctrl);
  void onEnter() override;
  void onExit() override;
  void update(uint32_t dt_ms) override;
  const char *name() const override;
private:
  uint32_t _elapsed = 0;
};

#endif // SLAVE_STATES_H
