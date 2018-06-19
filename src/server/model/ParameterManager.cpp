#include "ParameterManager.h"
#include "ProductManager.h"
#include "Message.h"
#include "Middleware.h"
#include "strategy/ClusterManager.h"
#include "config/EnvConfig.h"

ParameterManager* ParameterManager::GetInstance()
{
  static ParameterManager manager;
  return &manager;
}

void ParameterManager::InitGlobal()
{
  auto user = EnvConfig::GetInstance()->GetString(EnvVar::EXCHANGE);
  /// Sync Exchange
  {
    auto req = Message::NewProto<Proto::ExchangeParameterReq>();
    req->set_type(Proto::RequestType::Get);
    req->set_user(user);
    auto rep = std::dynamic_pointer_cast<Proto::ExchangeParameterRep>(
        Middleware::GetInstance()->Request(req));
    if (rep && rep->result().result())
    {
      exchange_ = std::make_shared<Exchange>();
      for(auto &p : rep->parameters())
      {
        exchange_->OnExchangeParameter(p);
      }
    }
    else
    {
      LOG_ERR << "Failed to sync Exchange.";
    }
  }
}

void ParameterManager::Init()
{
  auto user = EnvConfig::GetInstance()->GetString(EnvVar::EXCHANGE);
  // /// Sync Exchange
  // {
  //   auto req = Message::NewProto<Proto::ExchangeParameterReq>();
  //   req->set_type(Proto::RequestType::Get);
  //   req->set_user(user);
  //   auto rep = std::dynamic_pointer_cast<Proto::ExchangeParameterRep>(
  //       Middleware::GetInstance()->Request(req));
  //   if (rep && rep->result().result())
  //   {
  //     exchange_ = std::make_shared<Exchange>();
  //     for(auto &p : rep->parameters())
  //     {
  //       exchange_->OnExchangeParameter(p);
  //     }
  //   }
  //   else
  //   {
  //     LOG_ERR << "Failed to sync Exchange.";
  //   }
  // }
  /// Sync InterestRate
  {
    auto req = Message::NewProto<Proto::InterestRateReq>();
    req->set_type(Proto::RequestType::Get);
    req->set_user(user);
    auto rep = std::dynamic_pointer_cast<Proto::InterestRateRep>(
        Middleware::GetInstance()->Request(req));
    if (rep && rep->result().result())
    {
      std::lock_guard<std::mutex> lck(interest_rates_mtx_);
      interest_rates_.clear();
      for (auto &r : rep->rates())
      {
        interest_rates_[r.days()] = r.rate();
      }
    }
    else
    {
      LOG_ERR << "Failed to sync InterestRate.";
    }
  }

  /// Sync SSRate
  {
    auto req = Message::NewProto<Proto::SSRateReq>();
    req->set_type(Proto::RequestType::Get);
    req->set_user(user);
    auto rep = std::dynamic_pointer_cast<Proto::SSRateRep>(
        Middleware::GetInstance()->Request(req));
    if (rep && rep->result().result())
    {
      std::lock_guard<std::mutex> lck(ssrates_mtx_);
      for (auto &r : rep->rates())
      {
        auto *inst = ProductManager::GetInstance()->FindId(r.underlying());
        if (inst)
        {
          auto it = ssrates_.find(inst);
          if (it == ssrates_.end())
          {
            it = ssrates_.emplace(inst, std::make_shared<DateRateMap>()).first;
          }
          auto maturity = boost::gregorian::from_undelimited_string(r.maturity());
          (*it->second)[maturity] = r.rate();
        }
      }
    }
    else
    {
      LOG_ERR << "Failed to sync SSRate.";
    }
  }

  /// Sync VolatilityCurve
  {
    auto req = Message::NewProto<Proto::VolatilityCurveReq>();
    req->set_type(Proto::RequestType::Get);
    req->set_user(user);
    auto rep = std::dynamic_pointer_cast<Proto::VolatilityCurveRep>(
        Middleware::GetInstance()->Request(req));
    if (rep && rep->result().result())
    {
      std::lock_guard<std::mutex> lck(volatility_curves_mtx_);
      for (auto &v : rep->curves())
      {
        auto *inst = ProductManager::GetInstance()->FindId(v.underlying());
        if (inst)
        {
          auto it = volatility_curves_.find(inst);
          if (it == volatility_curves_.end())
          {
            it = volatility_curves_.emplace(inst, std::make_shared<DateVolatilityMap>()).first;
          }
          auto date = boost::gregorian::from_undelimited_string(v.maturity());
          auto curve = Message::NewProto<Proto::VolatilityCurve>();
          curve->CopyFrom(v);
          (*it->second)[date] = curve;
        }
      }
    }
    else
    {
      LOG_ERR << "Failed to sync VolatilityCurve.";
    }
  }

  /// Sync Destriker
  {
    auto req = Message::NewProto<Proto::DestrikerReq>();
    req->set_type(Proto::RequestType::Get);
    req->set_user(user);
    auto rep = std::dynamic_pointer_cast<Proto::DestrikerRep>(
        Middleware::GetInstance()->Request(req));
    if (rep && rep->result().result())
    {
      std::lock_guard<std::mutex> lck(destrikers_mtx_);
      for (auto &d : rep->destrikers())
      {
        auto *inst = ProductManager::GetInstance()->FindId(d.instrument());
        if (inst)
        {
          destrikers_[inst] = d.destriker();
        }
      }
    }
    else
    {
      LOG_ERR << "Failed to sync Destriker.";
    }
  }

  /// Sync Elastic
  // {
  //   auto req = Message::NewProto<Proto::ElasticReq>();
  //   req->set_type(Proto::RequestType::Get);
  //   req->set_user(user);
  //   auto rep = std::dynamic_pointer_cast<Proto::ElasticRep>(
  //       Middleware::GetInstance()->Request(req));
  //   if (rep && rep->result().result())
  //   {
  //     std::lock_guard<std::mutex> lck(elastics_mtx_);
  //     for (auto &e : rep->elastics())
  //     {
  //       auto *inst = ProductManager::GetInstance()->FindId(e.instrument());
  //       if (inst)
  //       {
  //         elastics_[inst] = e.elastic();
  //       }
  //     }
  //   }
  //   else
  //   {
  //     LOG_ERR << "Failed to sync Elastic.";
  //   }
  // }
}

bool ParameterManager::GetInterestRate(const boost::gregorian::date &date, double &rate)
{
  std::lock_guard<std::mutex> lck(interest_rates_mtx_);
  if (!interest_rates_.empty())
  {
    int32_t days = (date - boost::gregorian::day_clock::local_day()).days() + 1;
    auto it = interest_rates_.upper_bound(days);
    if (it == interest_rates_.begin())
    {
      rate = it->second;
    }
    else if (it != interest_rates_.end())
    {
      int32_t up_days = it->first;
      double up_rates = it->second;
      --it;
      rate = it->second + (up_rates - it->second) * (days - it->first) / (up_days - it->first);
    }
    else
    {
      rate = interest_rates_.rbegin()->second;
    }
    return true;
  }
  return false;
}

bool ParameterManager::GetSSRate(const Instrument *underlying, const boost::gregorian::date &date,
    double &rate)
{
  std::lock_guard<std::mutex> lck(ssrates_mtx_);
  auto it = ssrates_.find(underlying);
  if (it != ssrates_.end())
  {
    auto it1 = it->second->find(date);
    if (it1 != it->second->end())
    {
      rate = it1->second;
      return true;
    }
  }
  return false;
}

//bool ParameterManager::GetVolatility(const Instrument *instrument, double &volatility)
//{
//  std::lock_guard<std::mutex> lck(volatilities_mtx_);
//  auto it = volatilities_.find(instrument);
//  if (it != volatilities_.end())
//  {
//    volatility = it->second;
//    return true;
//  }
//  return false;
//}

std::shared_ptr<Proto::VolatilityCurve> ParameterManager::GetVolatilityCurve(
    const Instrument *instrument, const boost::gregorian::date &date)
{
  std::lock_guard<std::mutex> lck(volatility_curves_mtx_);
  auto it = volatility_curves_.find(instrument);
  if (it != volatility_curves_.end())
  {
    auto it1 = it->second->find(date);
    if (it1 != it->second->end())
    {
      return it1->second;
    }
  }
  return nullptr;
}

bool ParameterManager::GetDestriker(const Instrument *instrument, double &destriker)
{
  std::lock_guard<std::mutex> lck(destrikers_mtx_);
  auto it = destrikers_.find(instrument);
  if (it != destrikers_.end())
  {
    destriker = it->second;
    return true;
  }
  return false;
}

// bool ParameterManager::GetElastic(const Instrument *instrument, double &elastic)
// {
//   std::lock_guard<std::mutex> lck(elastics_mtx_);
//   auto it = elastics_.find(instrument);
//   if (it != elastics_.end())
//   {
//     elastic = it->second;
//     return true;
//   }
//   return false;
// }

ParameterManager::ProtoReplyPtr ParameterManager::OnExchangeParameterReq(
    const std::shared_ptr<Proto::ExchangeParameterReq> &req)
{
  if (req->type() == Proto::RequestType::Set)
  {
    for(auto &p : req->parameters())
    {
      exchange_->OnExchangeParameter(p);
    }
    ClusterManager::GetInstance()->Publish(req);
    LOG_PUB << req->user() << " set exchange parameters";
  }
  return nullptr;
}

ParameterManager::ProtoReplyPtr ParameterManager::OnInterestRateReq(
    const std::shared_ptr<Proto::InterestRateReq> &req)
{
  if (req->type() == Proto::RequestType::Set)
  {
    {
      std::lock_guard<std::mutex> lck(interest_rates_mtx_);
      interest_rates_.clear();
      for (auto &r : req->rates())
      {
        interest_rates_[r.days()] = r.rate();
      }
    }
    ClusterManager::GetInstance()->Publish(req);
    LOG_PUB << req->user() << " set interest rates";
  }
  return nullptr;
}

ParameterManager::ProtoReplyPtr ParameterManager::OnSSRateReq(
    const std::shared_ptr<Proto::SSRateReq> &req)
{
  auto type = req->type();
  if (type == Proto::RequestType::Set)
  {
    for (auto &r : req->rates())
    {
      auto *inst = ProductManager::GetInstance()->FindId(r.underlying());
      if (inst)
      {
        auto p = Message::NewProto<Proto::SSRate>();
        p->CopyFrom(r);
        auto *dm = ClusterManager::GetInstance()->FindDevice(inst);
        if (dm)
        {
          dm->Publish(p);
        }
        auto maturity = boost::gregorian::from_undelimited_string(r.maturity());
        std::lock_guard<std::mutex> lck(ssrates_mtx_);
        auto it = ssrates_.find(inst);
        if (it == ssrates_.end())
        {
          it = ssrates_.emplace(inst, std::make_shared<DateRateMap>()).first;
        }
        (*it->second)[maturity] = r.rate();
      }
      LOG_PUB << boost::format("%1% set ssrate of %2%@%3%") %
        req->user() % r.underlying() % r.maturity();
    }
  }
  else if (type == Proto::RequestType::Del)
  {
    for (auto &r : req->rates())
    {
      auto *inst = ProductManager::GetInstance()->FindId(r.underlying());
      if (inst)
      {
        std::lock_guard<std::mutex> lck(ssrates_mtx_);
        auto it = ssrates_.find(inst);
        if (it != ssrates_.end())
        {
          auto maturity = boost::gregorian::from_undelimited_string(r.maturity());
          it->second->erase(maturity);
        }
      }
      LOG_PUB << boost::format("%1% delete ssrate of %2%@%3%") %
        req->user() % r.underlying() % r.maturity();
    }
  }
  return nullptr;
}

//ParameterManager::ProtoReplyPtr ParameterManager::OnVolatilityReq(
//const std::shared_ptr<Proto::VolatilityReq> &req)
//{
//  auto type = req->type();
//  if (type == Proto::RequestType::Set)
//  {
//    std::lock_guard<std::mutex> lck(volatilities_mtx_);
//    for (auto &v : req->volatilities())
//    {
//      auto *inst = ProductManager::GetInstance()->FindId(v.instrument());
//      if (inst)
//      {
//        volatilities_[inst] = v.volatility();
//      }
//    }
//  }
//  else if (type == Proto::RequestType::Del)
//  {
//    std::lock_guard<std::mutex> lck(volatilities_mtx_);
//    for (auto &v : req->volatilities())
//    {
//      auto *inst = ProductManager::GetInstance()->FindId(v.instrument());
//      if (inst)
//      {
//        volatilities_.erase(inst);
//      }
//    }
//  }
//}

ParameterManager::ProtoReplyPtr ParameterManager::OnVolatilityCurveReq(
    const std::shared_ptr<Proto::VolatilityCurveReq> &req)
{
  auto type = req->type();
  if (type == Proto::RequestType::Set)
  {
    for (auto &vc : req->curves())
    {
      auto *inst = ProductManager::GetInstance()->FindId(vc.underlying());
      if (inst)
      {
        auto p = Message::NewProto<Proto::VolatilityCurve>();
        p->CopyFrom(vc);
        auto *dm = ClusterManager::GetInstance()->FindDevice(inst);
        if (dm)
        {
          dm->Publish(p);
        }
        auto date = boost::gregorian::from_undelimited_string(vc.maturity());
        std::lock_guard<std::mutex> lck(volatility_curves_mtx_);
        auto it = volatility_curves_.find(inst);
        if (it == volatility_curves_.end())
        {
          it = volatility_curves_.emplace(inst, std::make_shared<DateVolatilityMap>()).first;
        }
        (*it->second)[date] = p;
      }
      LOG_PUB << boost::format("%1% set volatility curve of %2%@%3%") %
        req->user() % vc.underlying() % vc.maturity();
    }
  }
  else if (type == Proto::RequestType::Del)
  {
    for (auto &vc : req->curves())
    {
      auto *inst = ProductManager::GetInstance()->FindId(vc.underlying());
      if (inst)
      {
        std::lock_guard<std::mutex> lck(volatility_curves_mtx_);
        auto it = volatility_curves_.find(inst);
        if (it != volatility_curves_.end())
        {
          it->second->erase(boost::gregorian::from_undelimited_string(vc.maturity()));
          if (it->second->empty())
          {
            volatility_curves_.erase(it);
          }
        }
      }
      LOG_PUB << boost::format("%1% delete volatility curve of %2%@%3%") %
        req->user() % vc.underlying() % vc.maturity();
    }
  }
  return nullptr;
}

ParameterManager::ProtoReplyPtr ParameterManager::OnDestrikerReq(
    const std::shared_ptr<Proto::DestrikerReq> &req)
{
  auto type = req->type();
  if (type == Proto::RequestType::Set)
  {
    std::set<const Instrument*> underlyings;
    {
      std::lock_guard<std::mutex> lck(destrikers_mtx_);
      for (auto &d : req->destrikers())
      {
        auto *inst = ProductManager::GetInstance()->FindId(d.instrument());
        if (inst)
        {
          destrikers_[inst] = d.destriker();
          underlyings.insert(inst->HedgeUnderlying());
        }
      }
    }
    for (auto *inst : underlyings)
    {
      auto *dm = ClusterManager::GetInstance()->FindDevice(inst);
      if (dm)
      {
        dm->Publish(req);
      }
    }
    LOG_PUB << req->user() << " set destrikers";
  }
  else if (type == Proto::RequestType::Del)
  {
    std::lock_guard<std::mutex> lck(destrikers_mtx_);
    for (auto &d : req->destrikers())
    {
      auto *inst = ProductManager::GetInstance()->FindId(d.instrument());
      if (inst)
      {
        destrikers_.erase(inst);
      }
    }
    LOG_PUB << req->user() << " delete destrikers";
  }
  return nullptr;
}

// ParameterManager::ProtoReplyPtr ParameterManager::OnElasticReq(const std::shared_ptr<Proto::ElasticReq> &req)
// {
//   auto type = req->type();
//   if (type == Proto::RequestType::Set)
//   {
//     std::lock_guard<std::mutex> lck(elastics_mtx_);
//     for (auto &e : req->elastics())
//     {
//       auto *inst = ProductManager::GetInstance()->FindId(e.instrument());
//       if (inst)
//       {
//         elastics_[inst] = e.elastic();
//       }
//     }
//   }
//   else if (type == Proto::RequestType::Del)
//   {
//     std::lock_guard<std::mutex> lck(elastics_mtx_);
//     for (auto &e : req->elastics())
//     {
//       auto *inst = ProductManager::GetInstance()->FindId(e.instrument());
//       if (inst)
//       {
//         elastics_.erase(inst);
//       }
//     }
//   }
// }
