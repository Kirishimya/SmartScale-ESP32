#ifndef STATE_FACTORY_H
#define STATE_FACTORY_H

#include <memory>
#include <string>

class State;
class StateMachine;
class ISlaveController;

std::unique_ptr<State> makeState(const std::string &stateName, StateMachine &sm, ISlaveController &ctrl);

#endif // STATE_FACTORY_H
