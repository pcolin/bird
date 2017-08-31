#ifndef BASE_DISRUPTOR_WAIT_STRATEGY_H
#define BASE_DISRUPTOR_WAIT_STRATEGY_H

#include "Sequence.h"
#include "SequenceBarrier.h"
#include <vector>

namespace base
{
class WaitStrategy
{
  public:
    WaitStrategy() {}
    virtual int64_t WaitFor(int64_t sequence,
                            const Sequence &cursor,
                            const std::vector<Sequence*> &dependent,
                            const SequenceBarrier &barrier) = 0;
    virtual void SignalAllWhenBlocking() = 0;
};
}

#endif
