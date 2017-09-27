#include <iostream>
#include <thread>
#include <atomic>
#include <sys/time.h>
#include "RingBuffer.h"
#include "YieldingWaitStrategy.h"
#include "BusySpinWaitStrategy.h"
#include "SingleProducerSequencer.h"
#include "MultiProducerSequencer.h"
#include "ProcessingSequenceBarrier.h"
#include "BatchEventProcessor.h"

#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <stdio.h>
using namespace base;
using namespace std;

const int size = 1024 * 32;
// const int64_t iterators = 10;
const int64_t iterators = 1000 * 1000 * 100;

void Publish(RingBuffer<int64_t, size> &rb, int n)
{
  // cout << n << " Begin of publish" << endl;
  const int sz = 100;
  for (int64_t i = 0; i < iterators;)
  {
    int64_t seq = rb.Next(sz);
    for (int j = sz-1; j >= 0; --j) rb.Get(seq-j) = i++;
    rb.Publish(seq - sz + 1, seq);
    // cout << n << " Publish " << seq << " " << rb.Get(seq) << endl;
  }
  // cout << n << " End of publish" << endl;
}

void ThreeToOne()
{
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(0, &cpuset);
  int ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  BusySpinWaitStrategy strategy;
  MultiProducerSequencer<size> sequencer(&strategy);
  RingBuffer<int64_t, size> rb(&sequencer);
  vector<Sequence*> sequences;
  SequenceBarrier *barrier = sequencer.NewBarrier(sequences);

  int64_t padding0[7] = {0};
  int64_t total1 = 0;
  int64_t padding1[7] = {0};
  int64_t total2 = 0;
  int64_t padding2[7] = {0};

  BatchEventProcessor<RingBuffer<int64_t, size>> p1(&rb, barrier,
      [&](int64_t &event, int64_t seq, bool last)
      {
        total1 += event;
        // cout << "1+= " << seq << " " << event << endl;
      });
  rb.AddGatingSequence(p1.GetSequence());
  thread t1(std::bind(&BatchEventProcessor<RingBuffer<int64_t, size>>::Run, &p1));
  cpu_set_t cpuset1;
  CPU_ZERO(&cpuset1);
  CPU_SET(1, &cpuset1);
  ret = pthread_setaffinity_np(t1.native_handle(), sizeof(cpu_set_t), &cpuset1);


  struct timeval start_time, end_time;
  gettimeofday(&start_time, 0);
  thread t2(std::bind(Publish, std::ref(rb), 1));
  cpu_set_t cpuset2;
  CPU_ZERO(&cpuset2);
  CPU_SET(2, &cpuset2);
  ret = pthread_setaffinity_np(t2.native_handle(), sizeof(cpu_set_t), &cpuset2);
  thread t3(std::bind(Publish, std::ref(rb), 2));
  cpu_set_t cpuset3;
  CPU_ZERO(&cpuset3);
  CPU_SET(3, &cpuset3);
  ret = pthread_setaffinity_np(t3.native_handle(), sizeof(cpu_set_t), &cpuset3);
  Publish(rb, 3);

  int64_t expect_count = 3 * iterators - 1;
  while (p1.GetSequence()->Get() != expect_count)
  {
    // cout << "consumer sequence : " << p1.GetSequence()->Get() << " " << expect_count << endl;
    std::this_thread::yield();
    // std::this_thread::sleep_for(std::chrono::microseconds(1));
  }
  gettimeofday(&end_time, 0);
  double start = start_time.tv_sec + ((double)start_time.tv_usec / 1000000);
  double end = end_time.tv_sec + ((double)end_time.tv_usec / 1000000);
  cout.precision(15);
  cout << iterators * 1.0 / (end - start) << " ops/secs" << endl;
  p1.Halt();
  // p2.Halt();

  t1.join();
  t2.join();
  t3.join();
  // t4.join();
  cout << "Total1 " << total1 << " " << iterators * (iterators-1) / 2 << endl;
}

void TwoToTwo()
{
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(0, &cpuset);
  int ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  BusySpinWaitStrategy strategy;
  MultiProducerSequencer<size> sequencer(&strategy);
  RingBuffer<int64_t, size> rb(&sequencer);
  vector<Sequence*> sequences;
  SequenceBarrier *barrier = sequencer.NewBarrier(sequences);

  int64_t padding0[7] = {0};
  int64_t total1 = 0;
  int64_t padding1[7] = {0};
  int64_t total2 = 0;
  int64_t padding2[7] = {0};

  BatchEventProcessor<RingBuffer<int64_t, size>> p1(&rb, barrier,
      [&](int64_t &event, int64_t seq, bool last)
      {
        total1 += event;
        // cout << "1+= " << seq << " " << event << endl;
      });
  rb.AddGatingSequence(p1.GetSequence());
  thread t1(std::bind(&BatchEventProcessor<RingBuffer<int64_t, size>>::Run, &p1));
  cpu_set_t cpuset1;
  CPU_ZERO(&cpuset1);
  CPU_SET(1, &cpuset1);
  ret = pthread_setaffinity_np(t1.native_handle(), sizeof(cpu_set_t), &cpuset1);

  BatchEventProcessor<RingBuffer<int64_t, size>> p2(&rb, barrier,
      [&](int64_t &event, int64_t seq, bool last)
      {
        total2 += event;
        // cout << "2+= " << seq << " " << event << endl;
      });
  rb.AddGatingSequence(p2.GetSequence());
  thread t2(std::bind(&BatchEventProcessor<RingBuffer<int64_t, size>>::Run, &p2));
  cpu_set_t cpuset2;
  CPU_ZERO(&cpuset2);
  CPU_SET(2, &cpuset2);
  ret = pthread_setaffinity_np(t2.native_handle(), sizeof(cpu_set_t), &cpuset2);

  struct timeval start_time, end_time;
  gettimeofday(&start_time, 0);
  thread t3(std::bind(Publish, std::ref(rb), 1));
  cpu_set_t cpuset3;
  CPU_ZERO(&cpuset3);
  CPU_SET(3, &cpuset3);
  ret = pthread_setaffinity_np(t3.native_handle(), sizeof(cpu_set_t), &cpuset3);
  // thread t4(std::bind(Publish, std::ref(rb), 2));
  Publish(rb, 2);

  int64_t expect_count = 2 * iterators - 1;
  while (p1.GetSequence()->Get() != expect_count && p2.GetSequence()->Get() != expect_count)
  {
    // cout << "consumer sequence : " << p1.GetSequence()->Get() << " " << expect_count << endl;
    std::this_thread::yield();
    // std::this_thread::sleep_for(std::chrono::microseconds(1));
  }
  gettimeofday(&end_time, 0);
  double start = start_time.tv_sec + ((double)start_time.tv_usec / 1000000);
  double end = end_time.tv_sec + ((double)end_time.tv_usec / 1000000);
  cout.precision(15);
  cout << iterators * 1.0 / (end - start) << " ops/secs" << endl;
  p1.Halt();
  p2.Halt();

  t1.join();
  t2.join();
  t3.join();
  // t4.join();
  cout << "Total1 " << total1 << " " << iterators * (iterators-1) / 2 << endl;
  cout << "Total2 " << total2 << " " << iterators * (iterators-1) / 2 << endl;
}

void OneToThree()
{
  // cout << "CPU num : " << sysconf(_SC_NPROCESSORS_CONF) << endl;
  // cout << "CPU num : " << std::thread::hardware_concurrency() << endl;
  // cout << "main thread pid : " << getpid() << " " << pthread_self() << endl;
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(0, &cpuset);
  int ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  cout << "affinity result : " << ret << endl;
  BusySpinWaitStrategy strategy;
  SingleProducerSequencer sequencer(size, &strategy);
  vector<Sequence*> sequences;
  RingBuffer<int64_t, size> rb(&sequencer);
  base::SequenceBarrier *barrier = sequencer.NewBarrier(sequences);

  int64_t padding0[7] = {0};
  int64_t total1 = 0;
  int64_t padding1[7] = {0};
  int64_t total2 = 0;
  int64_t padding2[7] = {0};
  int64_t total3 = 0;
  int64_t padding3[7] = {0};
  BatchEventProcessor<RingBuffer<int64_t, size>> processor1(&rb, barrier, [&](int64_t &event, int64_t seq, bool last)
      {
        total1 += event;
      });
  rb.AddGatingSequence(processor1.GetSequence());
  thread t(std::bind(&BatchEventProcessor<RingBuffer<int64_t, size>>::Run, &processor1));
  cpu_set_t cpuset1;
  CPU_ZERO(&cpuset1);
  CPU_SET(1, &cpuset1);
  ret = pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &cpuset1);
  cout << "affinity result : " << ret << endl;

  BatchEventProcessor<RingBuffer<int64_t, size>> processor2(&rb, barrier, [&](int64_t &event, int64_t seq, bool last)
      {
        total2 += event;
      });
  rb.AddGatingSequence(processor2.GetSequence());
  thread t2(std::bind(&BatchEventProcessor<RingBuffer<int64_t, size>>::Run, &processor2));
  cpu_set_t cpuset2;
  CPU_ZERO(&cpuset2);
  CPU_SET(2, &cpuset2);
  ret = pthread_setaffinity_np(t2.native_handle(), sizeof(cpu_set_t), &cpuset2);
  cout << "affinity result : " << ret << endl;

  BatchEventProcessor<RingBuffer<int64_t, size>> processor3(&rb, barrier, [&](int64_t &event, int64_t seq, bool last)
      {
        total3 += event;
      });
  rb.AddGatingSequence(processor3.GetSequence());
  thread t3(std::bind(&BatchEventProcessor<RingBuffer<int64_t, size>>::Run, &processor3));
  cpu_set_t cpuset3;
  CPU_ZERO(&cpuset3);
  CPU_SET(3, &cpuset3);
  ret = pthread_setaffinity_np(t3.native_handle(), sizeof(cpu_set_t), &cpuset3);
  cout << "affinity result : " << ret << endl;

  struct timeval start_time, end_time;
  gettimeofday(&start_time, 0);
  Publish(rb, 1);

  int64_t expect_count = iterators - 1;
  while (processor1.GetSequence()->Get() != expect_count || processor2.GetSequence()->Get() != expect_count || processor3.GetSequence()->Get() != expect_count)
  {
    // cout << "consumer sequence : " << sequence.Get() << " " << expect_count << endl;
    std::this_thread::yield();
    // std::this_thread::sleep_for(std::chrono::microseconds(1));
  }
  gettimeofday(&end_time, 0);
  double start = start_time.tv_sec + ((double)start_time.tv_usec / 1000000);
  double end = end_time.tv_sec + ((double)end_time.tv_usec / 1000000);
  cout.precision(15);
  cout << iterators * 1.0 / (end - start) << " ops/secs" << endl;
  processor1.Halt();
  processor2.Halt();
  processor3.Halt();

  t.join();
  t2.join();
  t3.join();
  cout << "Total1 " << total1 << " " << iterators * (iterators-1) / 2 << endl;
  cout << "Total2 " << total2 << " " << iterators * (iterators-1) / 2 << endl;
  cout << "Total3 " << total3 << " " << iterators * (iterators-1) / 2 << endl;
}

int main()
{
  OneToThree();
  TwoToTwo();
  ThreeToOne();

  return 0;
}
