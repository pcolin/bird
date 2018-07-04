#ifndef STRATEGY_STRATEGY_DEVICE_H
#define STRATEGY_STRATEGY_DEVICE_H

#include "StrategyTypes.h"
#include "base/disruptor/SequenceBarrier.h"

#include <thread>

class Strategy;
class StrategyDevice
{
public:
  StrategyDevice(std::unique_ptr<Strategy> &strategy, StrategyRingBuffer &rb,
      base::SequenceBarrier *barrier);
  ~StrategyDevice();

  void Start();
  void Stop(const std::string &reason);
  bool IsRunning() const;
  const std::string& Name() const;

private:
  void Run();

  std::unique_ptr<Strategy> strategy_;
  StrategyRingBuffer &rb_;
  base::SequenceBarrier *barrier_;
  base::Sequence sequence_;

  std::unique_ptr<std::thread> thread_;
  std::atomic<bool> running_;
};

#endif
