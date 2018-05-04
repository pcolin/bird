#ifndef BASE_DISRUPTOR_BUSY_SPIN_WAIT_STRATEGY_H
#define BASE_DISRUPTOR_BUSY_SPIN_WAIT_STRATEGY_H

#include "WaitStrategy.h"
// #include <iostream>

namespace base
{
class BusySpinWaitStrategy : public WaitStrategy
{
  public:
    virtual int64_t WaitFor(int64_t sequence,
                            const Sequence &cursor,
                            const std::vector<Sequence*> &dependents,
                            const SequenceBarrier &barrier) override
    {
      int64_t available_sequence = 0;
      if (dependents.empty())
      {
        while ((available_sequence = cursor.Get()) < sequence)
        {
          if (barrier.IsAlerted()) break;
          // std::cout << "BusySpin" << std::endl;
        }
      }
      else
      {
        while ((available_sequence = Sequence::GetMinimumSequence(dependents)) < sequence)
        {
          if (barrier.IsAlerted()) break;
          // std::cout << "BusySpinAll" << std::endl;
        }
      }
      return available_sequence;
    }

    virtual void SignalAllWhenBlocking() override {}

    BusySpinWaitStrategy() = default;
    BusySpinWaitStrategy(const BusySpinWaitStrategy&) = delete;
    BusySpinWaitStrategy& operator=(const BusySpinWaitStrategy&) = delete;
};
}

#endif
