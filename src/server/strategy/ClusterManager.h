#ifndef STRATEGY_CLUSTER_MANAGER_H
#define STRATEGY_CLUSTER_MANAGER_H

#include "Cash.pb.h"
#include "model/Instrument.h"

#include <unordered_map>
#include <memory>

class DeviceManager;
class ClusterManager
{
public:
  static ClusterManager* GetInstance();
  ~ClusterManager();

  void Init();

  DeviceManager* AddDevice(const Instrument *underlying);
  DeviceManager* FindDevice(const Instrument *underlying) const;

  void OnCash(const std::shared_ptr<PROTO::Cash> &cash);

private:
  ClusterManager();

  std::unordered_map<const Instrument*, DeviceManager*> devices_;
};

#endif
