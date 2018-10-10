#ifndef BASE_DISRUPTOR_PROCESSING_SEQUENCE_BARRIER_H
#define BASE_DISRUPTOR_PROCESSING_SEQUENCE_BARRIER_H

#include <memory>
#include "Sequencer.h"
#include "SequenceBarrier.h"
#include "WaitStrategy.h"

namespace base {

class ProcessingSequenceBarrier : public SequenceBarrier {
 public:
  ProcessingSequenceBarrier(Sequencer *sequencer, WaitStrategy *wait_strategy, Sequence *cursor,
      const std::vector<Sequence*> dependents)
    : wait_strategy_(wait_strategy), dependent_sequences_(dependents), cursor_(cursor),
    alerted_(false), sequencer_(sequencer) {}

  virtual int64_t WaitFor(int64_t sequence) override
  {
    /// FixME checkAlert()
    int64_t available = wait_strategy_->WaitFor(sequence, *cursor_, dependent_sequences_, *this);
    if (available < sequence) return available;
    return sequencer_->GetHighestSequence(sequence, available);
  }

  virtual int64_t GetCursor() const override
  {
    return Sequence::GetMinimumSequence(dependent_sequences_);
  }

  virtual bool IsAlerted() const override
  {
    return alerted_.load(std::memory_order::memory_order_acquire);
  }

  virtual void Alert() override
  {
    alerted_.store(true, std::memory_order::memory_order_release);
  }

  virtual void ClearAlert() override
  {
    alerted_.store(false, std::memory_order::memory_order_release);
  }

 private:
  WaitStrategy *wait_strategy_;
  std::vector<Sequence*> dependent_sequences_;
  std::atomic<bool> alerted_;
  Sequence *cursor_;
  Sequencer *sequencer_;
};

} // namespace base

#endif // BASE_DISRUPTOR_PROCESSING_SEQUENCE_BARRIER_H
