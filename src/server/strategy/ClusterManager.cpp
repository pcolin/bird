#include "ClusterManager.h"
#include "DeviceManager.h"
#include "base/common/Likely.h"
#include "base/logger/Logging.h"
#include "model/Middleware.h"
#include "model/ProductManager.h"
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
    auto rep = std::dynamic_pointer_cast<Proto::PricerRep>(Middleware::GetInstance()->Request(req));
    if (rep && rep->result().result()) {
      std::lock_guard<std::mutex> lck(pricer_mtx_);
      for (auto &p : rep->pricers()) {
        LOG_INF << "Pricer: " << p.ShortDebugString();
        auto pricer = Message::NewProto<Proto::Pricer>();
        pricer->CopyFrom(p);
        pricers_.emplace(p.name(), pricer);
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
        // auto *op = ProductManager::GetInstance()->FindId(s.option());
        // if (op)
        // {
        auto sw = Message::NewProto<Proto::StrategySwitch>();
        sw->CopyFrom(s);
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
    auto rep = std::dynamic_pointer_cast<Proto::CreditRep>(Middleware::GetInstance()->Request(req));
    if (rep && rep->result().result()) {
      std::lock_guard<std::mutex> lck(credit_mtx_);
      for (auto &c : rep->credits()) {
        LOG_INF << "Credit: " << c.ShortDebugString();
        // auto *underlying = ProductManager::GetInstance()->FindId(c.underlying());
        // if (underlying)
        // {
        auto credit = Message::NewProto<Proto::Credit>();
        credit->CopyFrom(c);
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
    auto rep = std::dynamic_pointer_cast<Proto::QuoterRep>(Middleware::GetInstance()->Request(req));
    if (rep && rep->result().result()) {
      std::lock_guard<std::mutex> lck(quoter_mtx_);
      for (auto &q : rep->quoters()) {
        LOG_INF << "Quoter: " << q.ShortDebugString();
        auto quoter = Message::NewProto<Proto::QuoterSpec>();
        quoter->CopyFrom(q);
        quoters_.emplace(q.name(), quoter);
      }
    } else {
      LOG_ERR << "Failed to sync quoters";
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
    auto ret = Message::NewProto<Proto::Pricer>();
    ret->CopyFrom(*it->second);
    return ret;
  }
  return nullptr;
}

std::shared_ptr<Proto::Pricer> ClusterManager::FindPricer(const Instrument *underlying) {
  std::lock_guard<std::mutex> lck(pricer_mtx_);
  for (auto &it : pricers_) {
    if (underlying->Id() == it.second->underlying()) {
      auto ret = Message::NewProto<Proto::Pricer>();
      ret->CopyFrom(*it.second);
      return ret;
    }
  }
  return nullptr;
}

std::shared_ptr<Proto::QuoterSpec> ClusterManager::FindQuoter(const std::string &name) {
  std::lock_guard<std::mutex> lck(quoter_mtx_);
  auto it = quoters_.find(name);
  if (it != quoters_.end()) {
    auto ret = Message::NewProto<Proto::QuoterSpec>();
    ret->CopyFrom(*it->second);
    return ret;
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
    LOG_ERR << "Cash isn't enough, stop all strategies...";
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
    auto *inst = ProductManager::GetInstance()->FindId(req->instrument());
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
        auto *underlying = ProductManager::GetInstance()->FindId(p.underlying());
        if (underlying) {
          auto copy = Message::NewProto<Proto::Pricer>();
          copy->CopyFrom(p);
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
        auto *underlying = ProductManager::GetInstance()->FindId(p.underlying());
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
      auto *underlying = ProductManager::GetInstance()->FindId(c.underlying());
      if (underlying) {
        auto credit = Message::NewProto<Proto::Credit>();
        credit->CopyFrom(c);
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
        auto *underlying = ProductManager::GetInstance()->FindId(q.underlying());
        if (underlying) {
          auto quoter = Message::NewProto<Proto::QuoterSpec>();
          quoter->CopyFrom(q);
          auto *dm = FindDevice(underlying);
          if (dm && dm->IsStrategyRunning(q.name())) {
            dm->Stop(q.name(), req->user() + " set quoter");
          }
          std::lock_guard<std::mutex> lck(quoter_mtx_);
          quoters_[q.name()] = quoter;
        }
        LOG_PUB << req->user() << " set Quoter " << q.name();
      }
    } else if (req->type() == Proto::RequestType::Del) {
      for (auto &q : req->quoters()) {
        auto *underlying = ProductManager::GetInstance()->FindId(q.underlying());
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
      auto *op = ProductManager::GetInstance()->FindId(s.option());
      if (op) {
        auto sw = Message::NewProto<Proto::StrategySwitch>();
        sw->CopyFrom(s);
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
      const Instrument *underlying = ProductManager::GetInstance()->FindId(op.underlying());
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
