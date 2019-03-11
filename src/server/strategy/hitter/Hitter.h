#ifndef STRATEGY_HITTER_H
#define STRATEGY_HITTER_H

#include "strategy/base/Strategy.h"
#include "Hitter.pb.h"
#include "Credit.pb.h"

class TraderApi;
class Hitter : public Strategy {
  struct Parameter {
    double credit;
    double multiplier;
    PriceLevel bid;
    PriceLevel ask;
    PriceLevel last;
    TheoMatrixPtr theos;
    int32_t bid_refills;
    int32_t ask_refills;
    bool bid_on;
    bool ask_on;
    bool is_cover;
    std::set<size_t> bids;
    std::set<size_t> asks;
  };
  typedef std::shared_ptr<Parameter> ParameterPtr;
  typedef std::unordered_map<const Instrument*, ParameterPtr> ParameterMap;

 public:
  Hitter(const std::string &name, DeviceManager *dm);

  virtual void OnStart() override;
  virtual void OnStop() override;

 protected:
  virtual void OnPrice(const PricePtr &price) override;
  virtual void OnTheoMatrix(const TheoMatrixPtr &theo) override;
  virtual void OnOrder(const OrderPtr &order) override;
  virtual void OnTrade(const TradePtr &trade) override;
  virtual bool OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat) override;

 private:
  bool OnPriceException(const std::shared_ptr<Proto::PriceException> &msg);
  bool OnCredit(const std::shared_ptr<Proto::Credit> &msg);
  bool OnStrategySwitch(const std::shared_ptr<Proto::StrategySwitch> &msg);
  bool OnStrategyOperate(const std::shared_ptr<Proto::StrategyOperate> &msg);
  bool OnInstrumentReq(const std::shared_ptr<Proto::InstrumentReq> &msg);
  void EvaluateLast(ParameterMap::iterator &it);
  void Evaluate(ParameterMap::iterator &it, Proto::HitType type);
  // void PublishStatistic();

  std::shared_ptr<Proto::HitterSpec> hitter_;
  ParameterMap parameters_;
  TraderApi *api_ = nullptr;

  base::VolumeType max_volume_;
  PricePtr underlying_price_;
  std::shared_ptr<Proto::PriceException> exception_;

  OrderPtr bid_;
  OrderPtr ask_;
  std::unordered_map<size_t, OrderPtr> orders_;
  std::shared_ptr<Proto::StrategyStatistic> statistic_;
  // int order_num_;
  // int trade_num_;
  // double delta_;
};

#endif
