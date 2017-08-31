#define BOOST_TEST_MODULE "DisruptorTest"

#include "base/disruptor/RingBuffer.h"
#include "base/disruptor/YieldingWaitStrategy.h"
#include "base/disruptor/SingleProducerSequencer.h"
#include "base/disruptor/ProcessingSequenceBarrier.h"
#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include <vector>
#include <thread>

using namespace std;

BOOST_AUTO_TEST_CASE(testPublish)
{
  // base::Sequence seq;
  const int size = 1024 * 64;
  const int iterators = 1000 * 1000 * 100;
  base::YieldingWaitStrategy strategy;
  base::SingleProducerSequencer sequencer(size, &strategy);
  vector<base::Sequence*> sequences;
  base::RingBuffer<int64_t, size> rb(&sequencer);
  base::Sequence sequence;
  rb.AddGatingSequence(&sequence);
  base::SequenceBarrier *barrier = sequencer.NewBarrier(sequences);

  int64_t seq = rb.Next();
  BOOST_CHECK_EQUAL(seq, 0);
  rb.Get(seq) = 8;
  rb.Publish(seq);

  seq = rb.Next();
  BOOST_CHECK_EQUAL(seq, 1);
  rb.Get(seq) = 5;
  rb.Publish(seq);

  // int64_t capacity = rb.RemainingCapacity();
  // BOOST_CHECK_EQUAL(capacity, size-2);

  // int64_t available = barrier->WaitFor(0);
  // BOOST_CHECK_EQUAL(available, 1);
  // BOOST_CHECK_EQUAL(rb.Get(0), 8);
  // BOOST_CHECK_EQUAL(rb.Get(1), 5);
}
/*
BOOST_AUTO_TEST_CASE(test1)
{
  // base::Sequence seq;
  const int size = 1024 * 64;
  const int iterators = 1000 * 1000 * 100;
  base::YieldingWaitStrategy strategy;
  base::SingleProducerSequencer sequencer(size, &strategy);
  vector<base::Sequence*> sequences;
  base::SequenceBarrier *barrier = sequencer.NewBarrier(sequences);
  base::RingBuffer<int64_t, size> rb(&sequencer);

  int64_t total = 0;
  base::Sequence sequence;
  std::atomic<bool> running(true);
  thread t([&]
      {
        int64_t next_sequence = sequence.Get() + 1L;
        while (running.load(std::memory_order::memory_order_acquire))
        {
          int64_t available_sequence = barrier->WaitFor(next_sequence);
          while (next_sequence <= available_sequence)
          {
            total += rb.Get(next_sequence);
          }
          sequence.Set(available_sequence);
        }
      });

  for (int i = 0; i < iterators; ++i)
  {
    int64_t seq = rb.Next();
    rb.Get(seq) = i;
    rb.Publish(seq);
  }

  int64_t expect_count = iterators - 1;
  while (sequence.Get() != expect_count)
  {
    std::this_thread::sleep_for(std::chrono::microseconds(1));
  }
  running.store(false, std::memory_order::memory_order_release);
  t.join();
  BOOST_CHECK_EQUAL(total, 1);
}
*/
