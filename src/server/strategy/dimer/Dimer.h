#ifndef STRATEGY_DIMER_H
#define STRATEGY_DIMER_H

#include "strategy/base/Strategy.h"
#include "Dimer.pb.h"
#include "SSRate.pb.h"
#include "Volatility.pb.h"
#include "InterestRate.pb.h"
#include "Credit.pb.h"
#include "Destriker.pb.h"

class TraderApi;
class Dimer : public Strategy {
  struct ParameterSide {
    bool on;
    bool canceling;
    int32_t refills;
    OrderPtr order;
    std::list<OrderPtr> other_orders;
  };

  struct Parameter {
    bool cover;
    double credit;
    double destriker;
    PricePtr price;
    TheoMatrixPtr theos;
    ParameterSide bid;
    ParameterSide ask;
  };
  typedef std::shared_ptr<Parameter> ParameterPtr;
  typedef std::unordered_map<const Instrument*, ParameterPtr> ParameterMap;

 public:
  Dimer(const std::string &name, DeviceManager *dm);

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
  bool OnSSRate(const std::shared_ptr<Proto::SSRate> &msg);
  bool OnVolatilityCurve(const std::shared_ptr<Proto::VolatilityCurve> &msg);
  bool OnInterestRateReq(const std::shared_ptr<Proto::InterestRateReq> &msg);
  bool OnCredit(const std::shared_ptr<Proto::Credit> &msg);
  bool OnDestriker(const std::shared_ptr<Proto::Destriker> &msg);
  bool OnStrategySwitch(const std::shared_ptr<Proto::StrategySwitch> &msg);
  bool OnStrategyOperate(const std::shared_ptr<Proto::StrategyOperate> &msg);

  void Cancel(ParameterSide &side, const char *reason);
  void Cancel(const TheoMatrixPtr &theos, ParameterSide &side,
      const std::function<bool(double, double)> &func, double credit, const char *reason);
  void Invalidate(ParameterMap::iterator &it);
  std::shared_ptr<Proto::DimerSpec> dimer_;
  ParameterMap parameters_;
  TraderApi *api_ = nullptr;

  OrderPtr bid_;
  OrderPtr ask_;
  std::unordered_map<size_t, OrderPtr> orders_;
  std::shared_ptr<Proto::StrategyStatistic> statistic_;
};

#endif
