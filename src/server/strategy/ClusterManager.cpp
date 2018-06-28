#include "ClusterManager.h"
#include "DeviceManager.h"
#include "base/common/Likely.h"
#include "base/logger/Logging.h"
#include "model/Middleware.h"
#include "model/ProductManager.h"
#include "model/CashManager.h"
#include "model/ParameterManager.h"
#include "config/EnvConfig.h"

// #include "Exchange.pb.h"

ClusterManager* ClusterManager::GetInstance()
{
  static ClusterManager manager;
  return &manager;
}

ClusterManager::ClusterManager()
{}

ClusterManager::~ClusterManager()
{
  for (auto it : devices_)
  {
    if (it.second != nullptr)
      delete it.second;
  }
}

void ClusterManager::Init()
{
  LOG_INF << "Initialize ClusterManager...";
  const std::string user = EnvConfig::GetInstance()->GetString(EnvVar::EXCHANGE);
  /// sync pricer from db.
  {
    auto req = Message::NewProto<Proto::PricerReq>();
    req->set_type(Proto::RequestType::Get);
    req->set_user(user);
    auto rep = std::dynamic_pointer_cast<Proto::PricerRep>(Middleware::GetInstance()->Request(req));
    if (rep && rep->result().result())
    {
      std::lock_guard<std::mutex> lck(pricer_mtx_);
      for (auto &p : rep->pricers())
      {
        LOG_INF << "Prcer: " << p.ShortDebugString();
        auto pricer = Message::NewProto<Proto::Pricer>();
        pricer->CopyFrom(p);
        pricers_.emplace(p.name(), pricer);
      }
    }
    else
    {
      LOG_ERR << "Failed to sync pricers";
    }
  }
  /// sync quoter from db.
  {
    auto req = Message::NewProto<Proto::QuoterReq>();
    req->set_type(Proto::RequestType::Get);
    req->set_user(user);
    auto rep = std::dynamic_pointer_cast<Proto::QuoterRep>(Middleware::GetInstance()->Request(req));
    if (rep && rep->result().result())
    {
      std::lock_guard<std::mutex> lck(quoter_mtx_);
      for (auto &q : rep->quoters())
      {
        LOG_INF << "Quoter: " << q.ShortDebugString();
        auto quoter = Message::NewProto<Proto::QuoterSpec>();
        quoter->CopyFrom(q);
        quoters_.emplace(q.name(), quoter);
      }
    }
    else
    {
      LOG_ERR << "Failed to sync quoters";
    }
  }
}

DeviceManager* ClusterManager::AddDevice(const Instrument *underlying)
{
  if (likely(underlying != nullptr))
  {
    auto it = devices_.find(underlying);
    if (it == devices_.end())
    {
      DeviceManager *device = new DeviceManager(underlying);
      device->Init();
      devices_.insert(std::make_pair(underlying, device));
      LOG_INF << "Create device manager for underlying " << underlying->Id();
      return device;
    }
    else
    {
      LOG_ERR << "Duplicated underlying " << underlying->Id();
      return it->second;
    }
  }
  return nullptr;
}

DeviceManager* ClusterManager::FindDevice(const Instrument *underlying) const
{
  if (likely(underlying != nullptr))
  {
    auto it = devices_.find(underlying);
    if (it != devices_.end())
    {
      return it->second;
    }
  }
  return nullptr;
}

std::shared_ptr<Proto::Pricer> ClusterManager::FindPricer(const std::string &name)
{
  std::lock_guard<std::mutex> lck(pricer_mtx_);
  auto it = pricers_.find(name);
  if (it != pricers_.end())
  {
    auto ret = Message::NewProto<Proto::Pricer>();
    ret->CopyFrom(*it->second);
    return ret;
  }
  return nullptr;
}

std::shared_ptr<Proto::Pricer> ClusterManager::FindPricer(const Instrument *underlying)
{
  std::lock_guard<std::mutex> lck(pricer_mtx_);
  for (auto &it : pricers_)
  {
    if (underlying->Id() == it.second->underlying())
    {
      auto ret = Message::NewProto<Proto::Pricer>();
      ret->CopyFrom(*it.second);
      return ret;
    }
  }
  return nullptr;
}

std::shared_ptr<Proto::QuoterSpec> ClusterManager::FindQuoter(const std::string &name)
{
  std::lock_guard<std::mutex> lck(quoter_mtx_);
  auto it = quoters_.find(name);
  if (it != quoters_.end())
  {
    auto ret = Message::NewProto<Proto::QuoterSpec>();
    ret->CopyFrom(*it->second);
    return ret;
  }
  return nullptr;
}

std::vector<std::shared_ptr<Proto::QuoterSpec>> ClusterManager::FindQuoters(
    const Instrument *underlying)
{
  std::vector<std::shared_ptr<Proto::QuoterSpec>> quoters;
  for (auto &it : quoters_)
  {
    if (it.second->underlying() == underlying->Id())
    {
      quoters.push_back(it.second);
    }
  }
  return quoters;
}

void ClusterManager::OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat)
{
  Publish(heartbeat);
}

void ClusterManager::OnInstrumentReq(const std::shared_ptr<Proto::InstrumentReq> &req)
{
  if (req->type() == Proto::RequestType::Set && req->instruments().size() > 0)
  {
    Publish(req);
    Middleware::GetInstance()->Publish(req);
  }
}

void ClusterManager::OnCash(const std::shared_ptr<Proto::Cash> &cash)
{
  if (!cash->is_enough())
  {
    LOG_ERR << "Cash isn't enough, stop all strategies...";
    StopAll();
  }
  CashManager::GetInstance()->OnCash(cash);
  Middleware::GetInstance()->Publish(cash);
}

ClusterManager::ProtoReplyPtr ClusterManager::OnPriceReq(const std::shared_ptr<Proto::PriceReq> &req)
{
  if (req->instrument().empty())
  {
    Publish(req);
  }
  else
  {
    auto *inst = ProductManager::GetInstance()->FindId(req->instrument());
    if (inst && inst->HedgeUnderlying())
    {
      auto it = devices_.find(inst->HedgeUnderlying());
      if (it != devices_.end())
      {
        it->second->Publish(req);
      }
    }
    else
    {
      LOG_ERR << "Can't find instrument " << req->instrument();
    }
  }
  return nullptr;
}

ClusterManager::ProtoReplyPtr ClusterManager::OnPricerReq(
    const std::shared_ptr<Proto::PricerReq> &req)
{
  if (req->type() != Proto::RequestType::Get)
  {
    if (req->type() == Proto::RequestType::Set)
    {
      for (auto &p : req->pricers())
      {
        auto *underlying = ProductManager::GetInstance()->FindId(p.underlying());
        if (underlying)
        {
          auto copy = Message::NewProto<Proto::Pricer>();
          copy->CopyFrom(p);
          auto *dm = FindDevice(underlying);
          if (dm)
          {
            dm->Publish(copy);
          }
          std::lock_guard<std::mutex> lck(pricer_mtx_);
          pricers_[p.name()] = copy;
        }
        LOG_PUB << req->user() << " set Pricer " << p.name();
      }
    }
    else if (req->type() == Proto::RequestType::Del)
    {
      std::lock_guard<std::mutex> lck(pricer_mtx_);
      for (auto &p : req->pricers())
      {
        pricers_.erase(p.name());
        LOG_PUB << req->user() << " delete Pricer " << p.name();
      }
    }
    // Middleware::GetInstance()->Publish(req);
  }
  return nullptr;
}

ClusterManager::ProtoReplyPtr ClusterManager::OnQuoterReq(
    const std::shared_ptr<Proto::QuoterReq> &req)
{
  if (req->type() != Proto::RequestType::Get)
  {
    // if (req->type() == Proto::RequestType::Set)
    // {
      for (auto &q : req->quoters())
      {
        auto *underlying = ProductManager::GetInstance()->FindId(q.underlying());
        if (underlying)
        {
          auto copy = Message::NewProto<Proto::QuoterSpec>();
          copy->CopyFrom(q);
          auto *dm = FindDevice(underlying);
          if (dm)
          {
            dm->OnQuoterSpec(req->user(), req->type(), copy);
          }
        }
        // LOG_PUB << req->user() << " set Quoter " << p.name();
      }
    // }
    // else if (req->type() == Proto::RequestType::Del)
    // {
    //   std::lock_guard<std::mutex> lck(quoter_mtx_);
    //   for (auto &p : req->quoters())
    //   {
    //     quoters_.erase(p.name());
    //     LOG_PUB << req->user() << " delete Quoter " << p.name();
    //   }
    // }
    // Middleware::GetInstance()->Publish(req);
  }
  return nullptr;
}

ClusterManager::ProtoReplyPtr ClusterManager::OnStrategyStatusReq(
    const std::shared_ptr<Proto::StrategyStatusReq> &req)
{
  if (req->type() == Proto::RequestType::Set)
  {
    for (auto &status : req->statuses())
    {
      const Instrument *underlying = ProductManager::GetInstance()->FindId(status.underlying());
      if (underlying)
      {
        auto *dm = FindDevice(underlying);
        if (dm)
        {
          dm->OnStrategyStatusReq(req);
        }
      }
      else
      {
        LOG_ERR << "Can't find underlying " << status.underlying();
      }
    }
    Publish(req);
    LOG_PUB << req->user() << " set StrategyStatus";
  }
  return nullptr;
}

bool ClusterManager::IsStrategiesRunning() const
{
  bool ret = false;
  for (auto &it : devices_)
  {
    if (it.second->IsStrategiesRunning())
    {
      ret = true;
    }
  }
  return ret;
}

void ClusterManager::StopAll()
{
  for (auto &it : devices_)
  {
    it.second->StopAll();
  }
}
