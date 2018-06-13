#ifndef STRATEGY_CLUSTER_MANAGER_H
#define STRATEGY_CLUSTER_MANAGER_H

#include "Heartbeat.pb.h"
#include "Reply.pb.h"
#include "Cash.pb.h"
#include "Pricer.pb.h"
#include "DeviceManager.h"
#include "model/Instrument.h"

#include <unordered_map>
#include <memory>

class ClusterManager
{
  typedef std::shared_ptr<Proto::Reply> ProtoReplyPtr;
public:
  static ClusterManager* GetInstance();
  ~ClusterManager();

  void Init();

  DeviceManager* AddDevice(const Instrument *underlying);
  DeviceManager* FindDevice(const Instrument *underlying) const;

  std::shared_ptr<Proto::Pricer> FindPricer(const std::string &name);
  std::shared_ptr<Proto::Pricer> FindPricer(const Instrument *underlying);

  void OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat);
  void OnInstrumentReq(const std::shared_ptr<Proto::InstrumentReq> &req);
  void OnCash(const std::shared_ptr<Proto::Cash> &cash);

  ProtoReplyPtr OnPriceReq(const std::shared_ptr<Proto::PriceReq> &req);
  ProtoReplyPtr OnPricerReq(const std::shared_ptr<Proto::PricerReq> &req);
  ProtoReplyPtr OnStrategyStatusReq(const std::shared_ptr<Proto::StrategyStatusReq> &req);

  template<class Type> void Publish(const Type &msg)
  {
    for (auto &it : devices_)
    {
      it.second->Publish(msg);
    }
  }

  bool IsStrategiesRunning() const;
  void StopAll();

private:
  ClusterManager();

  // void SyncPricer();
  // void SyncStrategySpec();

  std::unordered_map<const Instrument*, DeviceManager*> devices_;
  std::mutex pricer_mtx_;
  std::unordered_map<std::string, std::shared_ptr<Proto::Pricer>> pricers_;
};

#endif
