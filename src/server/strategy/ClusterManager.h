#ifndef STRATEGY_CLUSTER_MANAGER_H
#define STRATEGY_CLUSTER_MANAGER_H

#include "Heartbeat.pb.h"
#include "Reply.pb.h"
#include "Cash.pb.h"
#include "Pricer.pb.h"
#include "Credit.pb.h"
#include "Quoter.pb.h"
#include "DeviceManager.h"
#include "model/Instrument.h"

#include <unordered_map>
#include <memory>
// #include <boost/gre

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
  std::shared_ptr<Proto::QuoterSpec> FindQuoter(const std::string &name);
  std::vector<std::shared_ptr<Proto::QuoterSpec>> FindQuoters(const Instrument *underlying);

  void OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat);
  void OnInstrumentReq(const std::shared_ptr<Proto::InstrumentReq> &req);
  void OnCash(const std::shared_ptr<Proto::Cash> &cash);

  ProtoReplyPtr OnPriceReq(const std::shared_ptr<Proto::PriceReq> &req);
  ProtoReplyPtr OnPricerReq(const std::shared_ptr<Proto::PricerReq> &req);
  ProtoReplyPtr OnCreditReq(const std::shared_ptr<Proto::CreditReq> &req);
  ProtoReplyPtr OnQuoterReq(const std::shared_ptr<Proto::QuoterReq> &req);
  ProtoReplyPtr OnStrategySwitchReq(const std::shared_ptr<Proto::StrategySwitchReq> &req);
  ProtoReplyPtr OnStrategyOperateReq(const std::shared_ptr<Proto::StrategyOperateReq> &req);

  template<class Type> void Publish(const Type &msg)
  {
    for (auto &it : devices_)
    {
      it.second->Publish(msg);
    }
  }

  bool IsStrategiesRunning() const;
  void StopAll(const std::string &reason);

private:
  ClusterManager();

  std::unordered_map<const Instrument*, DeviceManager*> devices_;

  std::unordered_map<std::string, std::shared_ptr<Proto::Pricer>> pricers_;
  std::mutex pricer_mtx_;

  typedef std::unordered_map<const Instrument*,
          std::map<boost::gregorian::date, std::shared_ptr<Proto::Credit>>> CreditMap;
  std::vector<CreditMap> credits_;
  std::mutex credit_mtx_;

  typedef std::unordered_map<const Option*, std::shared_ptr<Proto::StrategySwitch>> SwitchMap;
  std::vector<SwitchMap> switches_;
  std::mutex switch_mtx_;

  std::unordered_map<std::string, std::shared_ptr<Proto::QuoterSpec>> quoters_;
  std::mutex quoter_mtx_;
};

#endif
