#ifndef BASE_DISRUPTOR_BLOCKING_WAIT_STRATEGY_H
#define BASE_DISRUPTOR_BLOCKING_WAIT_STRATEGY_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include "WaitStrategy.h"

namespace base {

class BlockingWaitStrategy : public WaitStrategy {
 public:
  virtual int64_t WaitFor(
      int64_t sequence,
      const Sequence &cursor,
      const std::vector<Sequence*> &dependents,
      const SequenceBarrier &barrier) override {
    int64_t available_sequence = 0;
    if (cursor.Get() < sequence) {
      std::unique_lock<std::recursive_mutex> lck(mutex_);
      while (cursor.Get() < sequence) {
        if (barrier.IsAlert()) break;
        cv_.wait(lck);
      }
    }
    if (!dependents.empty()) {
      while ((available_sequence = Sequence::GetMinimumSequence(dependents)) < sequence) {
        if (barrier.IsAlerted()) break;
      }
    }
    return available_sequence;
  }

  virtual void SignalAllWhenBlocking() override {
    std::lock_guard<std::recursive_mutex> lck(mutex_);
    cv_.notify_all();
  }

  BlockingWaitStrategy() = default;
  BlockingWaitStrategy(const BlockingWaitStrategy&) = delete;
  BlockingWaitStrategy& operator=(const BlockingWaitStrategy&) = delete;

 private:
  std::recursive_mutex mutex_;
  std::condition_variable_any cv_;
};

} // namespace base

#endif // BASE_DISRUPTOR_BLOCKING_WAIT_STRATEGY_H
