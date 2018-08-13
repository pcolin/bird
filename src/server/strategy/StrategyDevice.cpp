#include "StrategyDevice.h"
#include "Strategy.h"
#include "base/logger/Logging.h"
#include "model/Middleware.h"

#include <boost/format.hpp>

StrategyDevice::StrategyDevice(std::unique_ptr<Strategy> &strategy,
    StrategyRingBuffer &rb, base::SequenceBarrier *barrier)
  : strategy_(std::move(strategy)), rb_(rb), barrier_(barrier), running_(false)
{
  strategy_->Initialize([&](const std::string &reason)
      {
        LOG_PUB << strategy_->Name() << " will stop(" << reason << ')';
        running_ = false;
      });
}

StrategyDevice::~StrategyDevice()
{}

void StrategyDevice::Start()
{
  bool expected = false;
  if (running_.compare_exchange_strong(expected, true))
  {
    // thread_ = std::make_unique<std::thread>(&StrategyDevice::Run, this);
    // if (!thread_ || !thread_->joinable())
    // {
    //   thread_.reset(new std::thread(&StrategyDevice::Run, this));
    // }
    // else
    // {
    //   LOG_ERR << "thread is joinable.";
    // }
    if (thread_.joinable())
    {
      thread_.join();
    }
    thread_ = std::thread(&StrategyDevice::Run, this);
  }
}

void StrategyDevice::Stop(const std::string &reason)
{
  LOG_INF << boost::format("Stopping device %1%(%2%)") % strategy_->Name() % reason;
  bool expected = true;
  if (running_.compare_exchange_strong(expected, false))
  {
    LOG_DBG << "before alert";
    barrier_->Alert();
    thread_.join();
    LOG_DBG << "after alert";
  }
  // if (thread_)
  // {
  //   LOG_DBG << "before join";
  //   thread_->join();
  //   // thread_.reset();
  //   LOG_DBG << "Thread is joined.";
  // }
  LOG_PUB << boost::format("Device %1% was stopped(%2%)") % strategy_->Name() % reason;
}

bool StrategyDevice::IsRunning() const
{
  return running_;
}

const std::string& StrategyDevice::Name() const
{
  return strategy_->Name();
}

void StrategyDevice::Run()
{
  const std::string &name = strategy_->Name();
  LOG_INF << boost::format("Device %1% start running") % name;
  barrier_->ClearAlert();
  strategy_->OnStart();
  auto start = Message::NewProto<Proto::StrategyStatistic>();
  start->set_name(name);
  start->set_exchange(strategy_->Underlying()->Exchange());
  start->set_underlying(strategy_->UnderlyingId());
  start->set_status(Proto::StrategyStatus::Started);
  Middleware::GetInstance()->Publish(start);
  rb_.AddGatingSequence(&sequence_);

  int64_t begin = std::max(rb_.GetCursor(), 0l);
  sequence_.Set(begin - 1);
  int64_t next = begin;
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
  auto stop = Message::NewProto<Proto::StrategyStatistic>();
  stop->set_name(name);
  stop->set_exchange(strategy_->Underlying()->Exchange());
  stop->set_underlying(strategy_->UnderlyingId());
  stop->set_status(Proto::StrategyStatus::Stopped);
  Middleware::GetInstance()->Publish(stop);
  rb_.RemoveGatingSequence(&sequence_);
  LOG_INF << boost::format("Device %1% was stopped(seq:%2%->%3%)") % name % begin % next;
}
