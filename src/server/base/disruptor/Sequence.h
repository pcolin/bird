#ifndef BASE_DISRUPTOR_SEQUENCE_H
#define BASE_DISRUPTOR_SEQUENCE_H

#define CACHE_LINE_SIZE_IN_BYTES 64
#define ATOMIC_SEQUENCE_PADDING_LENGTH \
  (CACHE_LINE_SIZE_IN_BYTES - sizeof(std::atomic<int64_t>)) / sizeof(int64_t)

#include <atomic>
#include <memory>
#include <vector>
#include <climits>

namespace base
{
// constexpr int64_t kInitialCursorValue = -1L;

class Sequence
{
  public:
    static const int64_t INITIAL_VALUE_ = -1L;

    static int64_t GetMinimumSequence(const std::shared_ptr<std::vector<Sequence*>> &sequences,
        int64_t minimum = LONG_MAX)
    {
      return sequences ? GetMinimumSequence(*sequences) : minimum;
    }

    static int64_t GetMinimumSequence(const std::vector<Sequence*> &sequences,
        int64_t minimum = LONG_MAX)
    {
      for (Sequence *seq : sequences)
      {
        minimum = std::min(minimum, seq->Get());
      }
      return minimum;
    }

  public:
    Sequence(int64_t initial_value = INITIAL_VALUE_)
      : sequence_(initial_value) {}

    int64_t Get() const
    {
      return sequence_.load(std::memory_order_acquire);
    }

    void Set(int64_t value)
    {
      sequence_.store(value, std::memory_order_release);
    }

    int64_t IncrementAndGet(int64_t increment)
    {
      return sequence_.fetch_add(increment, std::memory_order_release) + increment;
    }

    bool CompareAndSet(int64_t expected_value, int64_t new_value)
    {
      return sequence_.compare_exchange_strong(expected_value, new_value, std::memory_order_acq_rel);
    }

  private:
    Sequence(const Sequence&) = delete;
    Sequence& operator=(const Sequence&) = delete;

    int64_t padding0_[ATOMIC_SEQUENCE_PADDING_LENGTH];
    std::atomic<int64_t> sequence_;
    int64_t padding1_[ATOMIC_SEQUENCE_PADDING_LENGTH];
};

}

#endif
