#include "DeviceManager.h"
#include "MarketMonitor.h"
#include "StrategyDevice.h"
#include "ClusterManager.h"
#include "strategy/pricer/Pricer.h"
#include "strategy/quoter/Quoter.h"
#include "strategy/hedger/Hedger.h"
#include "strategy/test/TestStrategy.h"
#include "config/EnvConfig.h"
#include "base/logger/Logging.h"
#include "base/common/Float.h"
#include "boost/format.hpp"

DeviceManager::DeviceManager(const Instrument *underlying)
    : underlying_(underlying),
      theo_(underlying),
      underlying_prices_(EnvConfig::GetInstance()->GetInt32(EnvVar::UL_PRICE_CHECK_NUM, 5)),
      sequencer_(&strategy_),
      rb_(&sequencer_),
      barrier_(sequencer_.NewBarrier(sequences_)) {}

DeviceManager::~DeviceManager() {
  if (barrier_) {
    delete barrier_;
    barrier_ = nullptr;
  }
}

void DeviceManager::Init() {
  /// initialize pricer
  auto pricer = ClusterManager::GetInstance()->FindPricer(underlying_);
  if (pricer) {
    theo_.SetParameter(pricer->theo_type(), pricer->elastic(), pricer->elastic_limit());
    max_price_change_ = pricer->warn_tick_change() * underlying_->Tick();

    std::unique_ptr<Strategy> strategy(new Pricer(pricer->name(), this));
    pricer_ = std::make_unique<StrategyDevice>(strategy, rb_, barrier_);
    // devices_.emplace(pricer->name(), std::move(d));
    LOG_INF << "add pricer " << pricer->name();
  }

  /// initialize quoter
  auto quoters = ClusterManager::GetInstance()->FindQuoters(underlying_);
  for (auto &quoter : quoters) {
    std::unique_ptr<Strategy> strategy(new Quoter(quoter->name(), this));
    auto d = std::make_shared<StrategyDevice>(strategy, rb_, barrier_);
    devices_.emplace(quoter->name(), std::move(d));
    LOG_INF << "add quoter " << quoter->name();
  }

  /// initialize hedger

  /// Sync config from db to be done...
  // std::unique_ptr<Strategy> strategy(new TestStrategy("test", this));
  // auto d = std::make_shared<StrategyDevice>(strategy, rb_, barrier_);
  // devices_.emplace("test", std::move(d));

  /// initialize market monitor
  const std::string name = (boost::format("%1%_monitor") % underlying_->Id()).str();
  std::unique_ptr<Strategy> monitor(new MarketMonitor(name, this));
  monitor_ = std::make_unique<StrategyDevice>(monitor, rb_, barrier_);
  monitor_->Start();
}

void DeviceManager::Publish(PricePtr &price) {
  /// add adjusted price to be done...
  if (price->instrument == underlying_) {
    double theo = theo_.ApplyElastic(price, hedger_ ? hedger_->OpenDelta() : 0);
    if (underlying_prices_.size() > 0) {
      double min = theo, max = theo;
      for (double p : underlying_prices_) {
        if (p < min)
          min = p;
        else if (p > max)
          max = p;
      }
      // if (underlying_->ConvertToTick(max - min) > warn_tick_change_) {
      if (base::IsMoreThan(max - min, max_price_change_)) {
        LOG_ERR << boost::format("%1% theo price warning: min(%2%), max(%3%)") %
                   underlying_->Id() % min % max;
        normal_ = false;
      } else if (normal_ == false) {
        normal_ = true;
        LOG_PUB << boost::format("%1% theo price is normal now: min(%2%), max(%3%)") %
                   underlying_->Id() % min % max;
      }
    }
    underlying_prices_.push_back(theo);
  }

  // Publish<PricePtr>(price);
  int64_t seq = rb_.Next();
  rb_.Get(seq) = price;
  rb_.Publish(seq);
}

void DeviceManager::Start(const std::string& name) {
  auto it = devices_.find(name);
  if (it != devices_.end()) {
    it->second->Start();
  }
}

void DeviceManager::StartAll() {
  for (auto &it : devices_) {
    it.second->Start();
  }
}

void DeviceManager::Stop(const std::string& name, const std::string &reason) {
  auto it = devices_.find(name);
  if (it != devices_.end()) {
    it->second->Stop(reason);
  }
}

void DeviceManager::StopAll(const std::string &reason) {
  for (auto &it : devices_) {
    it.second->Stop(reason);
  }
}

std::shared_ptr<StrategyDevice> DeviceManager::Find(const std::string &name) const {
  auto it = devices_.find(name);
  return it != devices_.end() ? it->second : nullptr;
}

void DeviceManager::Remove(const std::string &name) {
  auto it = devices_.find(name);
  if (it != devices_.end()) {
    if (it->second->IsRunning()) {
      it->second->Stop("delete quoter");
    }
    devices_.erase(it);
  }
}

void DeviceManager::OnStrategyOperate(const std::string &user, const Proto::StrategyOperate &op) {
  bool publish = false;
  auto sd = Find(op.name());
  if (sd) {
    if (sd->IsRunning()) {
      if (op.operate() == Proto::StrategyOperation::Stop) {
        sd->Stop(user + " stop");
      } else if (op.operate() == Proto::StrategyOperation::Start) {
        Publish(Message<Proto::StrategyOperate>::New(op));
        // copy->CopyFrom(op);
        // Publish(copy);
      }
    } else if (op.operate() == Proto::StrategyOperation::Start) {
      if (!pricer_->IsRunning()) {
        pricer_->Start();
      }
      sd->Start();
    }
  } else {
    LOG_ERR << "can't find strategy " << op.name();
  }
  LOG_PUB << boost::format("%1% %2% %3%") % user % Proto::StrategyOperation_Name(op.operate()) %
             op.name() ;
  // if (publish) Publish(msg);
}

// void DeviceManager::OnQuoterSpec(const std::string &user, Proto::RequestType type,
//     const std::shared_ptr<Proto::QuoterSpec> &quoter)
// {
//   LOG_INF << "On QuoterSpec: " << quoter->ShortDebugString();
//   const std::string &name = quoter->name();
//   if (type == Proto::RequestType::Set)
//   {
//     Publish(quoter);
//     // if (quoter->records().empty())
//     // {
//     //   std::lock_guard<std::mutex> lck(quoter_mtx_);
//     //   quoters_[name] = quoter;
//     // }
//     // else
//     // {
//     //   std::lock_guard<std::mutex> lck(quoter_mtx_);
//     //   auto it = quoter_records_.find(name);
//     //   if (it == quoter_records_.end())
//     //   {
//     //     it = quoter_records_.emplace(name,
//     //         std::unordered_map<std::string, std::shared_ptr<Proto::QuoterRecord>>()).first;
//     //   }
//     //   for (auto &r : quoter->records())
//     //   {
//     //     auto itr = it->second.find(r.instrument());
//     //     if (itr == it->second.end())
//     //     {
//     //       itr = it->second.emplace(r.instrument(), Message::NewProto<Proto::QuoterRecord>()).first;
//     //     }
//     //     if (r.credit() > 0 || r.multiplier() > 0)
//     //     {
//     //       itr->second->set_credit(r.credit());
//     //       itr->second->set_multiplier(r.multiplier());
//     //     }
//     //     else
//     //     {
//     //       itr->second->set_is_bid(r.is_bid());
//     //       itr->second->set_is_ask(r.is_ask());
//     //       itr->second->set_is_qr(r.is_qr());
//     //     }
//     //   }
//     // }
//     LOG_PUB << user << " set quoter " << quoter->name();
//   }
//   else if (type == Proto::RequestType::Del)
//   {
//     Stop(name, user + " delete quoter");
//     // {
//     //   std::lock_guard<std::mutex> lck(quoter_mtx_);
//     //   quoters_.erase(name);
//     //   quoter_records_.erase(name);
//     // }
//     LOG_PUB << user << " delete quoter " << name;
//   }
// }

bool DeviceManager::IsStrategyRunning(const std::string &name) const {
  auto it = devices_.find(name);
  return it != devices_.end() && it->second->IsRunning();
}

bool DeviceManager::IsStrategiesRunning() const {
  // int cnt = 0;
  for (auto &it : devices_) {
    if (it.second->IsRunning()) {
      return true;
      // ++cnt;
      // LOG_INF << it.second->Name() << " is running...";
    }
  }
  return false;
  // LOG_INF << boost::format("%1% has %2% strategies running") % underlying_->Id() % cnt;
  // return cnt > 0;
}

void DeviceManager::UpdatePricer(const Proto::Pricer &pricer) {
  theo_.SetParameter(pricer.theo_type(), pricer.elastic(), pricer.elastic_limit());
  max_price_change_ = pricer.warn_tick_change() * underlying_->Tick();
}

// std::shared_ptr<Proto::QuoterSpec> DeviceManager::GetQuoter(const std::string &name)
// {
//   std::lock_guard<std::mutex> lck(quoter_mtx_);
//   auto it = quoters_.find(name);
//   if (it != quoters_.end())
//   {
//     auto quoter = Message::NewProto<Proto::QuoterSpec>();
//     quoter->CopyFrom(*it->second);
//     auto itr = quoter_records_.find(name);
//     if (itr != quoter_records_.end())
//     {
//       for (auto &r : itr->second)
//       {
//         auto *record = quoter->add_records();
//         record->CopyFrom(*r.second);
//       }
//     }
//     return quoter;
//   }
//   return nullptr;
// }
