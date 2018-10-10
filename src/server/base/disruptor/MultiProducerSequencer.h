#ifndef BASE_DISRUPTOR_MULTI_PRODUCER_SEQUENCER_H
#define BASE_DISRUPTOR_MULTI_PRODUCER_SEQUENCER_H

#include <cmath>
#include <thread>
#include "Sequencer.h"
#include "WaitStrategy.h"

namespace base {

template<int N>
class MultiProducerSequencer : public Sequencer {
 public:
  MultiProducerSequencer(WaitStrategy *wait_strategy)
      : Sequencer(N, wait_strategy) {
    InitializeAvailableBuffer();
  }

  virtual bool HasAvailableCapacity(int required_capacity) override {
    int64_t cursor = cursor_.Get();
    int64_t wrap_point = cursor + required_capacity - buffer_size_;
    int64_t cached_gating_sequence = cached_sequence_.Get();
    if (wrap_point > cached_gating_sequence || cached_gating_sequence > cursor) {
      int64_t min_sequence = Sequence::GetMinimumSequence(
          std::atomic_load_explicit(&gating_sequences_, std::memory_order_acquire), cursor);
      cached_sequence_.Set(min_sequence);
      if (wrap_point > min_sequence) return false;
    }
    return true;
  }

  virtual int64_t RemainingCapacity() override {
    int64_t consumed = Sequence::GetMinimumSequence(
        std::atomic_load_explicit(&gating_sequences_, std::memory_order_acquire), cursor_.Get());
    int64_t produced = cursor_.Get();
    return buffer_size_ - produced + consumed;
  }

  virtual int64_t Next() override { return Next(1); }

  virtual int64_t Next(int n) override {
    assert(n > 0);
    int64_t current, next;
    do {
      current = cursor_.Get();
      next = current + n;

      int64_t wrap_point = next - buffer_size_;
      int64_t cached_gating_sequence = cached_sequence_.Get();
      if (wrap_point > cached_gating_sequence || cached_gating_sequence > next) {
        int64_t gating_sequence = Sequence::GetMinimumSequence(
            std::atomic_load_explicit(&gating_sequences_, std::memory_order_acquire), current);
        if (wrap_point > gating_sequence) {
          wait_strategy_->SignalAllWhenBlocking();
          std::this_thread::yield();
          continue;
        }
        cached_sequence_.Set(gating_sequence);
      } else if (cursor_.CompareAndSet(current, next)) {
        break;
      }
    } while(true);

    return next;
  }

  virtual void Claim(int64_t sequence) { cursor_.Set(sequence); }

  virtual void Publish(int64_t sequence) {
    SetAvailable(sequence);
    wait_strategy_->SignalAllWhenBlocking();
  }

  virtual void Publish(int64_t lo, int64_t hi) {
    for (int64_t i = lo; i <= hi; ++i) {
      SetAvailable(i);
    }
    wait_strategy_->SignalAllWhenBlocking();
  }

  virtual bool IsAvailable(int64_t sequence) {
    int idx = ((int)sequence) & (N - 1);
    assert(sequence >= 0);
    int flag = (int)(sequence >> index_shift_);
    return available_buffer_[idx].load(std::memory_order_acquire) == flag;
  }

  virtual int64_t GetHighestSequence(int64_t next_sequence, int64_t available_sequence) {
    for (int64_t i = next_sequence; i <= available_sequence; ++i) {
      if (!IsAvailable(i)) {
        return i - 1;
      }
    }
    return available_sequence;
  }

 private:
  void InitializeAvailableBuffer() {
    for (int i = N-1; i != 0; --i) {
      SetAvailableBufferValue(i, -1);
    }
    SetAvailableBufferValue(0, -1);
  }

  void SetAvailable(const int64_t sequence) {
    SetAvailableBufferValue(((int)sequence) & (N - 1), (int)(sequence >> index_shift_));
  }

  void SetAvailableBufferValue(int index, int flag) {
    available_buffer_[index].store(flag, std::memory_order_release);
  }

  Sequence cached_sequence_;
  std::atomic<int> available_buffer_[N];
  int index_shift_ = static_cast<int>(std::log2(N));
  // int64_t padding1_[7];
};

} // namespace base

#endif // BASE_DISRUPTOR_MULTI_PRODUCER_SEQUENCER_H
