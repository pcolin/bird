#include "DeviceManager.h"
#include "MarketMonitor.h"
#include "TestStrategy.h"
#include "StrategyDevice.h"
#include "Pricer.h"
#include "ClusterManager.h"
#include "config/EnvConfig.h"
#include "base/logger/Logging.h"

#include <boost/format.hpp>

DeviceManager::DeviceManager(const Instrument *underlying)
  : underlying_(underlying), theo_(underlying),
    underlying_prices_(EnvConfig::GetInstance()->GetInt32(EnvVar::UL_PRICE_CHECK_NUM, 5)),
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
  /// initialize pricing
  auto pricing = ClusterManager::GetInstance()->FindPricer(underlying_);
  if (pricing)
  {
    theo_.SetParameter(pricing->theo_type(), pricing->elastic(), pricing->elastic_limit());

    std::unique_ptr<Strategy> strategy(new Pricer(pricing->name(), this));
    auto d = std::make_shared<StrategyDevice>(strategy, rb_, barrier_);
    devices_.emplace(pricing->name(), std::move(d));
    LOG_INF << "Add pricer " << pricing->name();
  }

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

void DeviceManager::Publish(PricePtr &price)
{
  // int64_t seq = rb_.Next();
  // rb_.Get(seq) = std::move(price);
  // rb_.Publish(seq);

  /// add adjusted price to be done...
  if (price->instrument == underlying_)
  {
    double delta = 0; /// Get delta to be done...
    theo_.ApplyElastic(price, delta);
    double theo = theo_.Get();
    if (underlying_prices_.size() > 0)
    {
      double min = theo, max = theo;
      for (double p : underlying_prices_)
      {
        if (p < min)
          min = p;
        else if (p > max)
          max = p;
      }
      if (underlying_->ConvertToTick(max - min) > warn_tick_change_)
      {
        LOG_ERR << boost::format("%1% theo price warning: min(%2%), max(%3%)") %
          underlying_->Id() % min % max;
        normal_ = false;
      }
      else if (normal_ == false)
      {
        normal_ = true;
        LOG_PUB << boost::format("%1% theo price is normal now: min(%2%), max(%3%)") %
          underlying_->Id() % min % max;
      }
    }
    underlying_prices_.push_back(theo);
  }

  Publish<PricePtr>(price);
}

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

void DeviceManager::OnStrategyStatusReq(const std::shared_ptr<Proto::StrategyStatusReq> &msg)
{
  bool publish = false;
  for (auto &s : msg->statuses())
  {
    auto sd = FindStrategyDevice(s.name());
    if (sd)
    {
      if (sd->IsRunning())
      {
        if (s.status() == Proto::StrategyStatus::Stop)
        {
          sd->Stop();
        }
        else if (s.status() == Proto::StrategyStatus::Play)
        {
          publish = true;
        }
      }
      else if (s.status() == Proto::StrategyStatus::Play)
      {
        sd->Start();
        publish = true;
      }
    }
    LOG_PUB << boost::format("%1% set %2% : %3%") % msg->user() % s.name() %
      Proto::StrategyStatus::Status_Name(s.status());
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

void DeviceManager::UpdatePricer(const Proto::Pricer &pricing)
{
  theo_.SetParameter(pricing.theo_type(), pricing.elastic(), pricing.elastic_limit());
}
