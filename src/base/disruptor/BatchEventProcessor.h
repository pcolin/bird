#ifndef BASE_DISRUPTOR_BATCH_EVENT_PROCESSOR_H
#define BASE_DISRUPTOR_BATCH_EVENT_PROCESSOR_H

#include "SequenceBarrier.h"
// #include "RingBuffer.h"

#include <functional>

// #include <iostream>

namespace base
{
template<typename RingBufferType>
class BatchEventProcessor
{
    typedef std::function<void(class RingBufferType::EventType&, int64_t, bool)> OnEvent;
  public:

    BatchEventProcessor(RingBufferType *ring_buffer, SequenceBarrier *barrier, OnEvent on_event)
      : ring_buffer_(ring_buffer), barrier_(barrier), on_event_(on_event), running_(false)
    {}

    Sequence* GetSequence()
    {
      return &sequence_;
    }

    void Halt()
    {
      running_ = false;
      barrier_->Alert();
    }

    void Run()
    {
      bool expect = false;
      if (running_.compare_exchange_strong(expect, true))
      {
        barrier_->ClearAlert();
        int64_t next_sequence = sequence_.Get() + 1L;
        while (running_)
        {
          long available = barrier_->WaitFor(next_sequence);
          // std::cout << "Get available " << available << " " << next_sequence << std::endl;
          while (next_sequence <= available)
          {
            on_event_(ring_buffer_->Get(next_sequence), next_sequence, next_sequence == available);
            ++next_sequence;
          }
          sequence_.Set(available);
        }
      }
    }

  private:
    RingBufferType *ring_buffer_;
    SequenceBarrier *barrier_;
    Sequence sequence_;
    OnEvent on_event_;
    std::atomic<bool> running_;
};
}

#endif