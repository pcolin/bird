#ifndef BASE_DISRUPTOR_SLEEPING_WAIT_STRATEGY_H
#define BASE_DISRUPTOR_SLEEPING_WAIT_STRATEGY_H

#include "WaitStrategy.h"
#include <thread>

namespace base
{
class SleepingWaitStrategy : public WaitStrategy
{
  public:
    virtual int64_t WaitFor(int64_t sequence,
                            const Sequence &cursor,
                            const std::vector<Sequence*> &dependents,
                            const SequenceBarrier &barrier) override
    {
      int64_t available_sequence = 0;
      int count = RETRIES_;
      if (dependents.empty())
      {
        while ((available_sequence = cursor.Get()) < sequence)
        {
          if (barrier.IsAlerted()) break;
          count = ApplyWaitMethod(barrier, count);
        }
      }
      else
      {
        while ((available_sequence = Sequence::GetMinimumSequence(dependents)) < sequence)
        {
          if (barrier.IsAlerted()) break;
          count = ApplyWaitMethod(barrier, count);
        }
      }
      return available_sequence;
    }

    virtual void SignalAllWhenBlocking() override {}

    SleepingWaitStrategy() = default;
    SleepingWaitStrategy(const SleepingWaitStrategy&) = delete;
    SleepingWaitStrategy& operator=(const SleepingWaitStrategy&) = delete;

  private:
    int ApplyWaitMethod(const SequenceBarrier &barrier, int counter)
    {
      if (counter > 100)
      {
        --counter;
      }
      else if (counter > 0)
      {
        --counter;
        std::this_thread::yield();
      }
      else
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      return counter;
    }

    static const int RETRIES_ = 200;
};
}

#endif
