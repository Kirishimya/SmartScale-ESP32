#include <application/slave/StateMachine.h>
#include <application/ISlaveController.h>

StateMachine::StateMachine(ISlaveController &controller)
  : _controller(controller), _state(nullptr), _pending(nullptr) {}

StateMachine::~StateMachine() {
  if (_state) _state->onExit();
}

void StateMachine::transitionTo(std::unique_ptr<State> newState) {
  if (_updating) {
    _pending = std::move(newState);
    return;
  }
  applyTransition(std::move(newState));
}

void StateMachine::applyTransition(std::unique_ptr<State> newState) {
  if (_state) _state->onExit();
  _state = std::move(newState);
  if (_state) _state->onEnter();
}

void StateMachine::update(uint32_t dt_ms) {
  _updating = true;
  if (_state) _state->update(dt_ms);
  _updating = false;
  if (_pending) {
    auto next = std::move(_pending);
    applyTransition(std::move(next));
  }
}
