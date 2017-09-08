#ifndef STRATEGY_STRATEGY_DEVICE_H
#define STRATEGY_STRATEGY_DEVICE_H

#include "StrategyTypes.h"
#include "base/disruptor/SequenceBarrier.h"

#include <thread>

class Strategy;
class StrategyDevice
{
public:
  StrategyDevice(const std::string &name, std::unique_ptr<Strategy> &strategy, StrategyRingBuffer &rb,
      base::SequenceBarrier *barrier);
  ~StrategyDevice() {}

  void Start();
  void Stop();

  const std::string& Name() const
  {
    return name_;
  }

private:
  void Run();

  const std::string &name_;

  std::unique_ptr<Strategy> strategy_;
  StrategyRingBuffer &rb_;
  base::SequenceBarrier *barrier_;
  base::Sequence sequence_;

  std::unique_ptr<std::thread> thread_;
  std::atomic<bool> running_;
};

#endif
