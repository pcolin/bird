#ifndef BASE_DISRUPTOR_SINGLE_PRODUCER_SEQUENCER_H
#define BASE_DISRUPTOR_SINGLE_PRODUCER_SEQUENCER_H

#include "Sequencer.h"
#include "WaitStrategy.h"

namespace base
{
class SingleProducerSequencer : public Sequencer
{
  public:
    SingleProducerSequencer(int buffer_size, WaitStrategy *wait_strategy)
      : Sequencer(buffer_size, wait_strategy)
    {}

    virtual bool HasAvailableCapacity(int required_capacity) override;

    virtual int64_t RemainingCapacity() override;

    virtual int64_t Next() override
    {
      return Next(1);
    }

    virtual int64_t Next(int n) override;

    virtual void Claim(int64_t sequence)
    {
      this->next_value_ = sequence;
    }

    virtual void Publish(int64_t sequence)
    {
      cursor_.Set(sequence);
      wait_strategy_->SignalAllWhenBlocking();
    }

    virtual void Publish(int64_t lo, int64_t hi)
    {
      Publish(hi);
    }

    virtual bool IsAvailable(int64_t sequence)
    {
      return sequence <= cursor_.Get();
    }

    virtual int64_t GetHighestSequence(int64_t next_sequence, int64_t available_sequence)
    {
      return available_sequence;
    }

  private:
    int64_t padding0_[7];
    int64_t next_value_ = Sequence::INITIAL_VALUE_;
    int64_t cached_value_ = Sequence::INITIAL_VALUE_;
    int64_t padding1_[7];
};
}
#endif
