#include "DeviceManager.h"
#include "MarketMonitor.h"
#include "TestStrategy.h"
#include "StrategyDevice.h"

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
  std::unique_ptr<Strategy> strategy(new TestStrategy(this));
  auto d = std::make_shared<StrategyDevice>("test", strategy, rb_, barrier_);
  devices_.emplace("test", std::move(d));

  /// initialize market monitor
  std::unique_ptr<Strategy> monitor(new MarketMonitor(this));
  monitor_ = std::make_unique<StrategyDevice>("MarketMonitor", monitor, rb_, barrier_); 
  monitor_->Start();
}

void DeviceManager::Publish(PricePtr &price)
{
  int64_t seq = rb_.Next();
  rb_.Get(seq) = std::move(price);
  rb_.Publish(seq);
}
