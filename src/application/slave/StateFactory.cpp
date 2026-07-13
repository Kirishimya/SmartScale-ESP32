#include <application/slave/StateFactory.h>
#include <application/slave/State.h>
#include <application/slave/States.h>
#include <application/ISlaveController.h>
#include <application/slave/StateMachine.h>


std::unique_ptr<State> makeState(const std::string &stateName, StateMachine &sm, ISlaveController &ctrl) {
  if (stateName == "BOOT") return std::make_unique<BootState>(sm, ctrl);
  if (stateName == "INIT") return std::make_unique<InitState>(sm, ctrl);
  if (stateName == "DISCOVERY") return std::make_unique<DiscoveryState>(sm, ctrl);
  if (stateName == "WAIT_JOIN") return std::make_unique<WaitJoinState>(sm, ctrl);
  if (stateName == "TIME_SYNC") return std::make_unique<TimeSyncState>(sm, ctrl);
  if (stateName == "READY") return std::make_unique<ReadyState>(sm, ctrl);
  if (stateName == "WAIT_POLL") return std::make_unique<WaitPollState>(sm, ctrl);
  if (stateName == "SEND_DATA") return std::make_unique<SendDataState>(sm, ctrl);
  if (stateName == "WAIT_ACK") return std::make_unique<WaitAckState>(sm, ctrl);
  if (stateName == "IDLE") return std::make_unique<IdleState>(sm, ctrl);
  if (stateName == "SLEEP") return std::make_unique<SleepState>(sm, ctrl);
  if (stateName == "RECOVERY") return std::make_unique<RecoveryState>(sm, ctrl);
  return nullptr;
}
