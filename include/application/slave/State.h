#ifndef SLAVE_STATE_H
#define SLAVE_STATE_H

#include <cstdint>
#include <string>
#include <memory>

class StateMachine;
class ISlaveController;

class State {
public:
  State(StateMachine &sm, ISlaveController &ctrl) : _sm(sm), _ctrl(ctrl) {}
  virtual ~State() {}
  virtual void onEnter() = 0;
  virtual void onExit() = 0;
  virtual void update(uint32_t dt_ms) = 0;
  virtual const char* name() const = 0;
protected:
  StateMachine &_sm;
  ISlaveController &_ctrl;
};

#endif // SLAVE_STATE_H
