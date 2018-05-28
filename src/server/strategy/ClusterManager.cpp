#include "ClusterManager.h"
#include "DeviceManager.h"
#include "base/common/Likely.h"
#include "base/logger/Logging.h"
#include "model/Middleware.h"
#include "model/ProductManager.h"
#include "model/CashManager.h"

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
  /// sync pricing spec from db.
  {
    auto req = Message::NewProto<Proto::PricingSpecReq>();
    req->set_type(Proto::RequestType::Get);
    req->set_user(EnvConfig::GetInstance()->GetString(EnvVar::EXCHANGE));
    auto rep = std::dynamic_pointer_cast<Proto::PricingSpecRep>(
        Middleware::GetInstance()->Request(req));
    if (rep && rep->result().result())
    {
      std::lock_guard<std::mutex> lck(pricing_mtx_);
      for (auto &p : rep->pricings())
      {
        LOG_INF << "PrcingSpec: " << p.ShortDebugString();
        auto pricing = Message::NewProto<Proto::PricingSpec>();
        pricing->CopyFrom(p);
        pricings_.emplace(p.name(), pricing);
      }
    }
    else
    {
      LOG_ERR << "Failed to sync pricing specs";
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

std::shared_ptr<Proto::PricingSpec> ClusterManager::FindPricingSpec(const std::string &name)
{
  std::lock_guard<std::mutex> lck(pricing_mtx_);
  auto it = pricings_.find(name);
  if (it != pricings_.end())
  {
    auto ret = Message::NewProto<Proto::PricingSpec>();
    ret->CopyFrom(*it->second);
    return ret;
  }
  return nullptr;
}

std::shared_ptr<Proto::PricingSpec> ClusterManager::FindPricingSpec(const Instrument *underlying)
{
  std::lock_guard<std::mutex> lck(pricing_mtx_);
  for (auto &it : pricings_)
  {
    if (underlying->Id() == it.second->underlying())
    {
      auto ret = Message::NewProto<Proto::PricingSpec>();
      ret->CopyFrom(*it.second);
      return ret;
    }
  }
  return nullptr;
}

void ClusterManager::OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat)
{
  Publish(heartbeat);
  // for (auto &it : devices_)
  // {
  //   it.second->Publish(heartbeat);
  // }
}

void ClusterManager::OnInstrumentStatusUpdate(
    const std::shared_ptr<Proto::InstrumentStatusUpdate> &status)
{
  if (status->instruments().size() == 0)
  {
    Publish(status);
  }
  else
  {
    for (auto &s : status->instruments())
    {
      auto *inst = ProductManager::GetInstance()->FindId(s);
      if (inst && inst->HedgeUnderlying())
      {
        auto it = devices_.find(inst->HedgeUnderlying());
        if (it != devices_.end())
        {
          it->second->Publish(status);
        }
        else
        {
          LOG_ERR << "Can't find instrument " << s;
        }
      }
    }
  }
  Middleware::GetInstance()->Publish(status);
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

void ClusterManager::OnPriceReq(const std::shared_ptr<Proto::PriceReq> &req)
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
}

void ClusterManager::OnPricingSpecReq(const std::shared_ptr<Proto::PricingSpecReq> &req)
{
  if (req->type() != Proto::RequestType::Get)
  {
    if (req->type() == Proto::RequestType::Set)
    {
      for (auto &p : req->pricings())
      {
        auto *underlying = ProductManager::GetInstance()->FindId(p.underlying());
        if (underlying)
        {
          auto copy = Message::NewProto<Proto::PricingSpec>();
          copy->CopyFrom(p);
          auto *dm = FindDevice(underlying);
          if (dm)
          {
            dm->Publish(copy);
          }
          std::lock_guard<std::mutex> lck(pricing_mtx_);
          pricings_[p.name()] = copy;
        }
      }
    }
    else if (req->type() == Proto::RequestType::Del)
    {
      std::lock_guard<std::mutex> lck(pricing_mtx_);
      for (auto &p : req->pricings())
      {
        pricings_.erase(p.name());
      }
    }
    Middleware::GetInstance()->Publish(req);
  }
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
