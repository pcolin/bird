#include "SingleProducerSequencer.h"

#include <cassert>
#include <thread>
// #include <iostream>

using namespace base;

bool SingleProducerSequencer::HasAvailableCapacity(int required_capacity)
{
  int64_t next = this->next_value_;
  int64_t wrap_point = next + required_capacity - buffer_size_;
  int64_t cached_gating_sequence = this->cached_value_;
  if (wrap_point > cached_gating_sequence || cached_gating_sequence > next)
  {
    int64_t min_sequence = Sequence::GetMinimumSequence(
        std::atomic_load_explicit(&gating_sequences_, std::memory_order_acquire), next);
    this->cached_value_ = min_sequence;
    if (wrap_point > min_sequence) return false;
  }
  return true;
}

int64_t SingleProducerSequencer::RemainingCapacity()
{
  int64_t next = this->next_value_;
  // std::cout << "gating_sequences_ : " << (gating_sequences_ ? gating_sequences_->size() : 0) << std::endl;
  int64_t consumed = Sequence::GetMinimumSequence(
      std::atomic_load_explicit(&gating_sequences_, std::memory_order_acquire), next);
  // std::cout << "next_value_ : " << next_value_ << " cached_value_ : " << cached_value_ << std::endl;
  // std::cout << "consumed : " << consumed << std::endl;
  return buffer_size_ - next + consumed;
}

int64_t SingleProducerSequencer::Next(int n)
{
  assert(n > 0);
  int64_t next = this->next_value_;
  int64_t next_sequence = next + n;
  // std::cout << "next : " << next << " next_sequence : " << next_sequence << std::endl;
  int64_t wrap_point = next_sequence - buffer_size_;
  int64_t cached_gating_sequence = this->cached_value_;
  if (wrap_point > cached_gating_sequence || cached_gating_sequence > next)
  {
    // std::cout << "Wrap point : " << wrap_point << std::endl;
    cursor_.Set(next);
    int64_t min_sequence;
    while (wrap_point > (min_sequence = Sequence::GetMinimumSequence(
            std::atomic_load_explicit(&gating_sequences_, std::memory_order_acquire), next)))
    {
      wait_strategy_->SignalAllWhenBlocking();
      std::this_thread::yield(); /// to be done... parkNanos();
    }
    this->cached_value_ = min_sequence;
    // std::cout << "After Wrap point : " << min_sequence << std::endl;
  }
  this->next_value_ = next_sequence;
  // std::cout << "next_value_ : " << next_value_ << " cached_value_ : " << cached_value_ << std::endl;
  return next_sequence;
}
