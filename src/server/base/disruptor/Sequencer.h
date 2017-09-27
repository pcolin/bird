#ifndef BASE_DISRUPTOR_SEQUENCER_H
#define BASE_DISRUPTOR_SEQUENCER_H

#include "Sequence.h"
#include <vector>
#include <memory>

namespace base
{
class WaitStrategy;
class SequenceBarrier;
class Sequencer
{
  public:
    Sequencer(int buffer_size, WaitStrategy *wait_strategy)
      : buffer_size_(buffer_size), wait_strategy_(wait_strategy)
    {}

    int64_t GetCursor() const
    {
      return cursor_.Get();
    }

    int GetBufferSize() const
    {
      return buffer_size_;
    }

    virtual bool HasAvailableCapacity(int required_capacity) = 0;
    virtual int64_t RemainingCapacity() = 0;
    virtual int64_t Next() = 0;
    virtual int64_t Next(int n) = 0;
    virtual void Claim(int64_t sequence) = 0;
    virtual void Publish(int64_t sequence) = 0;
    virtual void Publish(int64_t lo, int64_t hi) = 0;
    virtual bool IsAvailable(int64_t sequence) = 0;
    virtual void AddGatingSequence(Sequence *sequence);
    virtual void RemoveGatingSequence(Sequence *sequence);
    SequenceBarrier* NewBarrier(const std::vector<Sequence*> &sequences);
    virtual int64_t GetMinimumSequence();
    virtual int64_t GetHighestSequence(int64_t next_sequence, int64_t available_sequence) = 0;

  protected:
    const int buffer_size_;
    WaitStrategy *wait_strategy_;
    Sequence cursor_;
    std::shared_ptr<std::vector<Sequence*>> gating_sequences_;
};
}
#endif
