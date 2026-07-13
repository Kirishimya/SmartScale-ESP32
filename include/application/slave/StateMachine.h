#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <application/slave/State.h>
#include <memory>

class StateMachine {
public:
  StateMachine(ISlaveController &controller);
  ~StateMachine();
  void transitionTo(std::unique_ptr<State> newState);
  void update(uint32_t dt_ms);
  State* current() { return _state.get(); }
private:
  ISlaveController &_controller;
  std::unique_ptr<State> _state;
  std::unique_ptr<State> _pending;
  bool _updating = false;
  void applyTransition(std::unique_ptr<State> newState);
};

#endif // STATEMACHINE_H
