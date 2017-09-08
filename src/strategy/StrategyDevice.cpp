#include "StrategyDevice.h"
#include "Strategy.h"
#include "base/logger/Logging.h"

#include <boost/format.hpp>

StrategyDevice::StrategyDevice(const std::string &name, std::unique_ptr<Strategy> &strategy,
    StrategyRingBuffer &rb, base::SequenceBarrier *barrier)
  : name_(name), strategy_(std::move(strategy)), rb_(rb), barrier_(barrier), running_(false)
{}

void StrategyDevice::Start()
{
  bool expected = false;
  if (running_.compare_exchange_strong(expected, true))
  {
    thread_ = std::make_unique<std::thread>(&StrategyDevice::Run, this);
  }
}

void StrategyDevice::Stop()
{
  LOG_INF << "Stopping device " << name_;
  bool expected = true;
  if (running_.compare_exchange_strong(expected, false))
  {
    barrier_->Alert();
  }
  if (thread_)
  {
    thread_->join();
    thread_.reset();
  }
  LOG_INF << boost::format("Device %1% was stopped") % name_;
}

void StrategyDevice::Run()
{
  LOG_INF << boost::format("Device %1% start running") % name_;
  barrier_->ClearAlert();
  strategy_->OnStart();
  rb_.AddGatingSequence(&sequence_);

  int64_t next = std::max(rb_.GetCursor(), 0l);
  while (running_)
  {
    int64_t available = barrier_->WaitFor(next);
    while (next <= available)
    {
      strategy_->OnEvent(rb_.Get(next), next, next == available);
      ++next;
    }
    sequence_.Set(available);
  }

  strategy_->OnStop();
  rb_.RemoveGatingSequence(&sequence_);
  LOG_INF << boost::format("Device %1% was stopped(seq:%2%)") % name_ % next;
}
