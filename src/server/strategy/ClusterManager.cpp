#include "ClusterManager.h"
#include "DeviceManager.h"
#include "base/common/Likely.h"
#include "base/logger/Logging.h"
#include "model/Middleware.h"
#include "model/CashManager.h"

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
  /// to be done... sync from db.
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

void ClusterManager::OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat)
{
  for (auto &it : devices_)
  {
    it.second->Publish(heartbeat);
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
