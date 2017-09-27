#ifndef BASE_DISRUPTOR_YIELDING_WAIT_STRATEGY_H
#define BASE_DISRUPTOR_YIELDING_WAIT_STRATEGY_H

#include "WaitStrategy.h"
#include <thread>
// #include <iostream>

namespace base
{
class YieldingWaitStrategy : public WaitStrategy
{
  public:
    virtual int64_t WaitFor(int64_t sequence,
                            const Sequence &cursor,
                            const std::vector<Sequence*> &dependents,
                            const SequenceBarrier &barrier) override
    {
      int64_t available_sequence = 0;
      int count = SPIN_TRIES_;
      if (dependents.empty())
      {
        while ((available_sequence = cursor.Get()) < sequence)
        {
          // std::cout << "empty available sequence " << available_sequence << " " << sequence << std::endl;
          if (barrier.IsAlerted()) break;
          count = ApplyWaitMethod(barrier, count);
        }
      }
      else
      {
        while ((available_sequence = Sequence::GetMinimumSequence(dependents)) < sequence)
        {
          // std::cout << "available sequence " << available_sequence << std::endl;
          if (barrier.IsAlerted()) break;
          count = ApplyWaitMethod(barrier, count);
        }
      }
      return available_sequence;
    }

    virtual void SignalAllWhenBlocking() override {}

    YieldingWaitStrategy() = default;
    YieldingWaitStrategy(const YieldingWaitStrategy&) = delete;
    YieldingWaitStrategy& operator=(const YieldingWaitStrategy&) = delete;

  private:
    int ApplyWaitMethod(const SequenceBarrier &barrier, int counter)
    {
      if (counter == 0)
      {
        std::this_thread::yield();
        // std::cout << "Yielded" << std::endl;
      }
      else
      {
        --counter;
      }
      return counter;
    }

    static const int SPIN_TRIES_ = 100;
};
}

#endif
