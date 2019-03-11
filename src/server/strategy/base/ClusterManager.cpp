#include "ClusterManager.h"
#include "DeviceManager.h"
#include "base/common/Likely.h"
#include "base/logger/Logging.h"
#include "model/Middleware.h"
#include "model/InstrumentManager.h"
#include "model/CashManager.h"
#include "model/ParameterManager.h"
#include "config/EnvConfig.h"
#include "boost/format.hpp"

ClusterManager* ClusterManager::GetInstance() {
  static ClusterManager manager;
  return &manager;
}

ClusterManager::ClusterManager()
    : credits_(Proto::StrategyType::DummyQuoter),
      switches_(Proto::StrategyType::DummyQuoter + 1) {}

ClusterManager::~ClusterManager() {
  for (auto it : devices_) {
    if (it.second != nullptr) delete it.second;
  }
}

void ClusterManager::Init() {
  LOG_INF << "Initialize ClusterManager...";
  const std::string user = EnvConfig::GetInstance()->GetString(EnvVar::EXCHANGE);
  /// sync pricer from db.
  {
    Proto::PricerReq req;
    req.set_type(Proto::RequestType::Get);
    req.set_user(user);
    auto rep = std::dynamic_pointer_cast<Proto::PricerRep>(
        Middleware::GetInstance()->Request(req));
    if (rep && rep->result().result()) {
      std::lock_guard<std::mutex> lck(pricer_mtx_);
      for (auto &p : rep->pricers()) {
        LOG_INF << "Pricer: " << p.ShortDebugString();
        pricers_.emplace(p.name(), Message<Proto::Pricer>::New(p));
      }
    } else {
      LOG_ERR << "Failed to sync pricers";
    }
  }

  /// sync strategy switch from db.
  {
    Proto::StrategySwitchReq req;
    req.set_type(Proto::RequestType::Get);
    req.set_user(user);
    auto rep = std::dynamic_pointer_cast<Proto::StrategySwitchRep>(
               Middleware::GetInstance()->Request(req));
    if (rep && rep->result().result()) {
      std::lock_guard<std::mutex> lck(switch_mtx_);
      for (auto &s : rep->switches()) {
        // auto *op = InstrumentManager::GetInstance()->FindId(s.option());
        // if (op)
        // {
        auto sw = Message<Proto::StrategySwitch>::New(s);
        // sw->CopyFrom(s);
        switches_[s.strategy()][s.option()] = sw;
        // }
      }
    } else {
      LOG_ERR << "Failed to sync strategy switches";
    }
  }

  /// sync credit from db.
  {
    Proto::CreditReq req;
    req.set_type(Proto::RequestType::Get);
    req.set_user(user);
    auto rep = std::dynamic_pointer_cast<Proto::CreditRep>(
        Middleware::GetInstance()->Request(req));
    if (rep && rep->result().result()) {
      std::lock_guard<std::mutex> lck(credit_mtx_);
      for (auto &c : rep->credits()) {
        LOG_INF << "Credit: " << c.ShortDebugString();
        // auto *underlying = InstrumentManager::GetInstance()->FindId(c.underlying());
        // if (underlying)
        // {
        auto credit = Message<Proto::Credit>::New(c);
        // credit->CopyFrom(c);
        auto date = boost::gregorian::from_undelimited_string(credit->maturity());
        credits_[c.strategy()][c.underlying()][date] = credit;
        LOG_DBG << boost::format("Add credits of %1%@%2%") % c.underlying() % c.maturity();
        // }
        // else
        // {
        //   LOG_ERR << "Can't find underlying " << c.underlying();
        // }
      }
    } else {
      LOG_ERR << "Failed to sync credits";
    }
  }

  /// sync quoter from db.
  {
    Proto::QuoterReq req;
    req.set_type(Proto::RequestType::Get);
    req.set_user(user);
    auto rep = std::dynamic_pointer_cast<Proto::QuoterRep>(
        Middleware::GetInstance()->Request(req));
    if (rep && rep->result().result()) {
      std::lock_guard<std::mutex> lck(quoter_mtx_);
      for (auto &q : rep->quoters()) {
        LOG_INF << "Quoter: " << q.ShortDebugString();
        quoters_.emplace(q.name(), Message<Proto::QuoterSpec>::New(q));
      }
    } else {
      LOG_ERR << "Failed to sync quoters";
    }
  }

  /// sync hitter from db.
  {
    Proto::HitterReq req;
    req.set_type(Proto::RequestType::Get);
    req.set_user(user);
    auto rep = std::dynamic_pointer_cast<Proto::HitterRep>(
        Middleware::GetInstance()->Request(req));
    if (rep && rep->result().result()) {
      std::lock_guard<std::mutex> lck(hitter_mtx_);
      for (auto &h : rep->hitters()) {
        LOG_INF << "Hitter: " << h.ShortDebugString();
        hitters_.emplace(h.name(), Message<Proto::HitterSpec>::New(h));
      }
    } else {
      LOG_ERR << "Failed to sync hitters";
    }
  }

  /// sync dimer from db.
  {
    Proto::DimerReq req;
    req.set_type(Proto::RequestType::Get);
    req.set_user(user);
    auto rep = std::dynamic_pointer_cast<Proto::DimerRep>(
        Middleware::GetInstance()->Request(req));
    if (rep && rep->result().result()) {
      std::lock_guard<std::mutex> lck(dimer_mtx_);
      for (auto &d : rep->dimers()) {
        LOG_INF << "Dimer: " << d.ShortDebugString();
        dimers_.emplace(d.name(), Message<Proto::DimerSpec>::New(d));
      }
    } else {
      LOG_ERR << "Failed to sync dimers";
    }
  }

  /// sync hedger from db.
  {
    Proto::HedgerReq req;
    req.set_type(Proto::RequestType::Get);
    req.set_user(user);
    auto rep = std::dynamic_pointer_cast<Proto::HedgerRep>(
        Middleware::GetInstance()->Request(req));
    if (rep && rep->result().result()) {
      std::lock_guard<std::mutex> lck(hedger_mtx_);
      for (auto &h : rep->hedgers()) {
        LOG_INF << "Hedger: " << h.ShortDebugString();
        hedgers_.emplace(h.name(), Message<Proto::HedgerSpec>::New(h));
      }
    } else {
      LOG_ERR << "Failed to sync hedgers";
    }
  }

  /// sync statistics from db.
  {
    Proto::MarketMakingStatisticReq req;
    req.set_type(Proto::RequestType::Get);
    req.set_user(user);
    auto rep = std::dynamic_pointer_cast<Proto::MarketMakingStatisticRep>(
        Middleware::GetInstance()->Request(req));
    if (rep && rep->result().result()) {
      for (auto &s : rep->statistics()) {
        LOG_INF << "MarketMakingStatistic: " << s.ShortDebugString();
        statistics_.emplace(s.underlying(), Message<Proto::MarketMakingStatistic>::New(s));
      }
    } else {
      LOG_ERR << "Failed to sync statistics";
    }
  }
}

DeviceManager* ClusterManager::AddDevice(const Instrument *underlying) {
  if (likely(underlying != nullptr)) {
    auto it = devices_.find(underlying);
    if (it == devices_.end()) {
      DeviceManager *device = new DeviceManager(underlying);
      device->Init();
      devices_.insert(std::make_pair(underlying, device));
      LOG_INF << "Create device manager for underlying " << underlying->Id();
      return device;
    } else {
      LOG_ERR << "Duplicated underlying " << underlying->Id();
      return it->second;
    }
  }
  return nullptr;
}

DeviceManager* ClusterManager::FindDevice(const Instrument *underlying) const {
  if (likely(underlying != nullptr)) {
    auto it = devices_.find(underlying);
    if (it != devices_.end()) {
      return it->second;
    }
  }
  return nullptr;
}

std::shared_ptr<Proto::Pricer> ClusterManager::FindPricer(const std::string &name) {
  std::lock_guard<std::mutex> lck(pricer_mtx_);
  auto it = pricers_.find(name);
  if (it != pricers_.end()) {
    return it->second;
  }
  return nullptr;
}

std::shared_ptr<Proto::Pricer> ClusterManager::FindPricer(const Instrument *underlying) {
  std::lock_guard<std::mutex> lck(pricer_mtx_);
  for (auto &it : pricers_) {
    if (underlying->Id() == it.second->underlying()) {
      return it.second;
      // ret->CopyFrom(*it.second);
      // return ret;
    }
  }
  return nullptr;
}

std::shared_ptr<Proto::QuoterSpec> ClusterManager::FindQuoter(const std::string &name) {
  std::lock_guard<std::mutex> lck(quoter_mtx_);
  auto it = quoters_.find(name);
  if (it != quoters_.end()) {
    return it->second;
  }
  return nullptr;
}

std::vector<std::shared_ptr<Proto::QuoterSpec>> ClusterManager::FindQuoters(
    const Instrument *underlying) {
  std::vector<std::shared_ptr<Proto::QuoterSpec>> quoters;
  std::lock_guard<std::mutex> lck(quoter_mtx_);
  for (auto &it : quoters_) {
    if (it.second->underlying() == underlying->Id()) {
      quoters.push_back(it.second);
    }
  }
  return quoters;
}

std::shared_ptr<Proto::HitterSpec> ClusterManager::FindHitter(const std::string &name) {
  std::lock_guard<std::mutex> lck(hitter_mtx_);
  auto it = hitters_.find(name);
  if (it != hitters_.end()) {
    return it->second;
  }
  return nullptr;
}

std::shared_ptr<Proto::DimerSpec> ClusterManager::FindDimer(const std::string &name) {
  std::lock_guard<std::mutex> lck(dimer_mtx_);
  auto it = dimers_.find(name);
  if (it != dimers_.end()) {
    return it->second;
  }
  return nullptr;
}

std::shared_ptr<Proto::HedgerSpec> ClusterManager::FindHedger(const std::string &name) {
  std::lock_guard<std::mutex> lck(hedger_mtx_);
  auto it = hedgers_.find(name);
  if (it != hedgers_.end()) {
    return it->second;
  }
  return nullptr;
}

std::vector<std::shared_ptr<Proto::Credit>> ClusterManager::FindCredits(
    Proto::StrategyType strategy, const Instrument *underlying) {
  std::vector<std::shared_ptr<Proto::Credit>> credits;
  std::lock_guard<std::mutex> lck(credit_mtx_);
  auto it = credits_[strategy].find(underlying->Id());
  if (it != credits_[strategy].end()) {
    for (auto &c : it->second) {
      credits.push_back(c.second);
    }
  }
  return credits;
}

std::shared_ptr<Proto::StrategySwitch> ClusterManager::FindStrategySwitch(
    Proto::StrategyType strategy, const Instrument* op) {
  std::lock_guard<std::mutex> lck(switch_mtx_);
  auto it = switches_[strategy].find(op->Id());
  if (it != switches_[strategy].end()) {
    return it->second;
  }
  return nullptr;
}

std::shared_ptr<Proto::MarketMakingStatistic> ClusterManager::FindStatistic(
    const std::string &underlying) {
  return statistics_[underlying];
}

void ClusterManager::OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat) {
  Publish(heartbeat);
}

void ClusterManager::OnInstrumentReq(const std::shared_ptr<Proto::InstrumentReq> &req) {
  if (req->type() == Proto::RequestType::Set && req->instruments().size() > 0) {
    Publish(req);
    Middleware::GetInstance()->Publish(req);
  }
}

void ClusterManager::OnCash(const std::shared_ptr<Proto::Cash> &cash) {
  if (!cash->is_enough()) {
    LOG_ERR << "cash isn't enough, stop all strategies...";
    StopAll("cash limit broken");
  }
  CashManager::GetInstance()->OnCash(cash);
  Middleware::GetInstance()->Publish(cash);
}

ClusterManager::ProtoReplyPtr ClusterManager::OnPriceReq(
    const std::shared_ptr<Proto::PriceReq> &req) {
  if (req->instrument().empty()) {
    Publish(req);
  } else {
    auto *inst = InstrumentManager::GetInstance()->FindId(req->instrument());
    if (inst && inst->HedgeUnderlying()) {
      auto it = devices_.find(inst->HedgeUnderlying());
      if (it != devices_.end()) {
        it->second->Publish(req);
      }
    } else {
      LOG_ERR << "Can't find instrument " << req->instrument();
    }
  }
  return nullptr;
}

ClusterManager::ProtoReplyPtr ClusterManager::OnPricerReq(
    const std::shared_ptr<Proto::PricerReq> &req) {
  if (req->type() != Proto::RequestType::Get) {
    if (req->type() == Proto::RequestType::Set) {
      for (auto &p : req->pricers()) {
        auto *underlying = InstrumentManager::GetInstance()->FindId(p.underlying());
        if (underlying) {
          auto copy = Message<Proto::Pricer>::New(p);
          // copy->CopyFrom(p);
          {
            std::lock_guard<std::mutex> lck(pricer_mtx_);
            pricers_[p.name()] = copy;
          }
          auto *dm = FindDevice(underlying);
          if (dm) {
            dm->Publish(copy);
          }
        }
        LOG_PUB << req->user() << " set Pricer " << p.name();
      }
    } else if (req->type() == Proto::RequestType::Del) {
      for (auto &p : req->pricers()) {
        auto *underlying = InstrumentManager::GetInstance()->FindId(p.underlying());
        if (underlying) {
          auto *dm = FindDevice(underlying);
          if (dm) {
            dm->StopAll(req->user() + " delete pricer");
          }
          std::lock_guard<std::mutex> lck(pricer_mtx_);
          pricers_.erase(p.name());
        }
        LOG_PUB << req->user() << " delete Pricer " << p.name();
      }
    }
  }
  return nullptr;
}

ClusterManager::ProtoReplyPtr ClusterManager::OnCreditReq(
    const std::shared_ptr<Proto::CreditReq> &req) {
  if (req->type() == Proto::RequestType::Set) {
    for (auto &c : req->credits()) {
      auto *underlying = InstrumentManager::GetInstance()->FindId(c.underlying());
      if (underlying) {
        auto credit = Message<Proto::Credit>::New(c);
        // credit->CopyFrom(c);
        auto date = boost::gregorian::from_undelimited_string(c.maturity());
        {
          std::lock_guard<std::mutex> lck(credit_mtx_);
          credits_[c.strategy()][underlying->Id()][date] = credit;
        }
        auto *dm = FindDevice(underlying);
        if (dm && dm->IsStrategiesRunning()) {
          dm->Publish(credit);
        }
      }
      LOG_PUB << boost::format("%1% set credit of %2%@%3%") %
                 req->user() % c.underlying() % c.maturity();
    }
  }
  return nullptr;
}

ClusterManager::ProtoReplyPtr ClusterManager::OnQuoterReq(
    const std::shared_ptr<Proto::QuoterReq> &req) {
  if (req->type() != Proto::RequestType::Get) {
    if (req->type() == Proto::RequestType::Set) {
      for (auto &q : req->quoters()) {
        auto *underlying = InstrumentManager::GetInstance()->FindId(q.underlying());
        if (underlying) {
          auto quoter = Message<Proto::QuoterSpec>::New(q);
          // quoter->CopyFrom(q);
          auto *dm = FindDevice(underlying);
          if (dm && dm->IsStrategyRunning(q.name())) {
            dm->Stop(q.name(), req->user() + " set quoter");
            dm->Publish(quoter);
          }
          std::lock_guard<std::mutex> lck(quoter_mtx_);
          quoters_[q.name()] = quoter;
        }
        LOG_PUB << req->user() << " set Quoter " << q.name();
      }
    } else if (req->type() == Proto::RequestType::Del) {
      for (auto &q : req->quoters()) {
        auto *underlying = InstrumentManager::GetInstance()->FindId(q.underlying());
        if (underlying) {
          auto *dm = FindDevice(underlying);
          if (dm) {
            dm->Remove(q.name());
          }
          std::lock_guard<std::mutex> lck(quoter_mtx_);
          quoters_.erase(q.name());
        }
        LOG_PUB << req->user() << " delete Quoter " << q.name();
      }
    }
  }
  return nullptr;
}

ClusterManager::ProtoReplyPtr ClusterManager::OnStrategySwitchReq(
    const std::shared_ptr<Proto::StrategySwitchReq> &req) {
  if (req->type() == Proto::RequestType::Set) {
    for (auto &s : req->switches()) {
      auto *op = InstrumentManager::GetInstance()->FindId(s.option());
      if (op) {
        auto sw = Message<Proto::StrategySwitch>::New(s);
        // sw->CopyFrom(s);
        {
          std::lock_guard<std::mutex> lck(switch_mtx_);
          switches_[s.strategy()][op->Id()] = sw;
        }
        auto *dm = FindDevice(op->HedgeUnderlying());
        if (dm && dm->IsStrategiesRunning()) {
          dm->Publish(sw);
        }
      } else {
        LOG_ERR << "Can't find option " << s.option();
      }
    }
    LOG_PUB << req->user() << " set StrategySwitch";
  }
  return nullptr;
}

ClusterManager::ProtoReplyPtr ClusterManager::OnStrategyOperateReq(
    const std::shared_ptr<Proto::StrategyOperateReq> &req) {
  if (req->type() == Proto::RequestType::Set) {
    for (auto &op : req->operates()) {
      const Instrument *underlying = InstrumentManager::GetInstance()->FindId(op.underlying());
      if (underlying) {
        auto *dm = FindDevice(underlying);
        if (dm) {
          dm->OnStrategyOperate(req->user(), op);
        }
      } else {
        LOG_ERR << "Can't find underlying " << op.underlying();
      }
    }
    // Publish(req);
    // LOG_PUB << req->user() << " set StrategyStatus";
  }
  return nullptr;
}

bool ClusterManager::IsStrategiesRunning() const {
  for (auto &it : devices_) {
    if (it.second->IsStrategiesRunning()) {
      return true;
    }
  }
  return false;
}

void ClusterManager::StopAll(const std::string &reason) {
  for (auto &it : devices_) {
    it.second->StopAll(reason);
  }
}
