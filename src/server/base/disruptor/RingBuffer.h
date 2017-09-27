#ifndef BASE_DISRUPTOR_RING_BUFFER_H
#define BASE_DISRUPTOR_RING_BUFFER_H

#include "Sequencer.h"
#include <iostream>
#include <cassert>

namespace base
{
template<class Event, std::size_t N>
class RingBuffer
{
  public:
    typedef Event EventType;

    RingBuffer(Sequencer *sequencer)
      : sequencer_(sequencer) {}

    Event& Get(const int64_t sequence)
    {
      return events_[sequence & (N - 1)];
    }

    int64_t GetCursor()
    {
      return sequencer_->GetCursor();
    }

    int64_t Next()
    {
      return sequencer_->Next();
    }

    int64_t Next(int n)
    {
      return sequencer_->Next(n);
    }

    void Publish(int64_t sequence)
    {
      sequencer_->Publish(sequence);
    }

    void Publish(int64_t lo, int64_t hi)
    {
      sequencer_->Publish(lo, hi);
    }

    SequenceBarrier* NewBarrier(const std::vector<Sequence*> &sequences)
    {
      return sequencer_->NewBarrier(sequences);
    }

    void AddGatingSequence(Sequence *sequence)
    {
      assert(sequence);
      sequencer_->AddGatingSequence(sequence);
    }

    void RemoveGatingSequence(Sequence *sequence)
    {
      assert(sequence);
      sequencer_->RemoveGatingSequence(sequence);
    }

    int64_t RemainingCapacity()
    {
      // int64_t tmp = sequencer_->RemainingCapacity();
      // std::cout << "RemainingCapacity() : " << tmp << std::endl;
      // return tmp;
      return sequencer_->RemainingCapacity();
    }

  private:
    int64_t padding0_[2 * 64];
    Event events_[N];
    int64_t padding1_[2 * 64];
    Sequencer *sequencer_;
    int64_t padding2_[7];
};

}

#endif
