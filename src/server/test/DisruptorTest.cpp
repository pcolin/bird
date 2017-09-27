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

BOOST_AUTO_TEST_CASE(testDisruptor)
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
