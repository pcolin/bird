#include "DeviceManager.h"
#include "MarketMonitor.h"
#include "TestStrategy.h"
#include "StrategyDevice.h"
#include "base/logger/Logging.h"

#include <boost/format.hpp>

// using namespace base;

DeviceManager::DeviceManager(const Instrument *underlying)
  : underlying_(underlying),
    sequencer_(&strategy_), rb_(&sequencer_), barrier_(sequencer_.NewBarrier(sequences_))
{}

DeviceManager::~DeviceManager()
{
  if (barrier_)
  {
    delete barrier_;
    barrier_ = nullptr;
  }
}

void DeviceManager::Init()
{
  /// Sync config from db to be done...
  std::unique_ptr<Strategy> strategy(new TestStrategy("test", this));
  auto d = std::make_shared<StrategyDevice>(strategy, rb_, barrier_);
  devices_.emplace("test", std::move(d));

  /// initialize market monitor
  const std::string name = (boost::format("%1%_monitor") % underlying_->Id()).str();
  std::unique_ptr<Strategy> monitor(new MarketMonitor(name, this));
  monitor_ = std::make_unique<StrategyDevice>(monitor, rb_, barrier_);
  monitor_->Start();
}

// void DeviceManager::Publish(PricePtr &price)
// {
//   int64_t seq = rb_.Next();
//   rb_.Get(seq) = std::move(price);
//   rb_.Publish(seq);
// }

void DeviceManager::Start(const std::string& name)
{
  auto it = devices_.find(name);
  if (it != devices_.end())
  {
    it->second->Start();
  }
}

void DeviceManager::StartAll()
{
  for (auto &it : devices_)
  {
    it.second->Start();
  }
}

void DeviceManager::Stop(const std::string& name)
{
  auto it = devices_.find(name);
  if (it != devices_.end())
  {
    it->second->Stop();
  }
}

void DeviceManager::StopAll()
{
  for (auto &it : devices_)
  {
    it.second->Stop();
  }
}

std::shared_ptr<StrategyDevice> DeviceManager::FindStrategyDevice(const std::string &name) const
{
  auto it = devices_.find(name);
  return it != devices_.end() ? it->second : nullptr;
}

void DeviceManager::OnStrategyStatusReq(const std::shared_ptr<proto::StrategyStatusReq> &msg)
{
  bool publish = false;
  for (auto &s : msg->statuses())
  {
    auto sd = FindStrategyDevice(s.name());
    if (sd)
    {
      if (sd->IsRunning())
      {
        if (s.status() == proto::StrategyStatus::Stop)
        {
          sd->Stop();
        }
        else if (s.status() == proto::StrategyStatus::Play)
        {
          publish = true;
        }
      }
      else if (s.status() == proto::StrategyStatus::Play)
      {
        sd->Start();
        publish = true;
      }
    }
    LOG_PUB << boost::format("%1% set %2% : %3%") % msg->user() % s.name() %
      proto::StrategyStatus::Status_Name(s.status());
  }
  if (publish) Publish(msg);
}

bool DeviceManager::IsStrategiesRunning() const
{
  int cnt = 0;
  for (auto &it : devices_)
  {
    if (it.second->IsRunning())
    {
      ++cnt;
      LOG_INF << it.second->Name() << " is running...";
    }
  }
  LOG_INF << boost::format("%1% has %2% strategies running") % underlying_->Id() % cnt;
  return cnt > 0;
}
