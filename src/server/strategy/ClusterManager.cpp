#include "ClusterManager.h"
#include "DeviceManager.h"
#include "base/common/Likely.h"
#include "base/logger/Logging.h"
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

void ClusterManager::OnCash(const std::shared_ptr<PROTO::Cash> &cash)
{
  if (!cash->is_enough())
  {
    for (auto &it : devices_)
    {
      it.second->StopAll();
    }
  }
  CashManager::GetInstance()->OnCash(cash);
  /// to be done
  // Middleware::GetInstance()->Publish(cash);
}
