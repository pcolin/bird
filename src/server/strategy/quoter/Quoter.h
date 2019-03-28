#ifndef STRATEGY_QUOTER_H
#define STRATEGY_QUOTER_H

#include <queue>
#include "strategy/base/Strategy.h"
#include "SSRate.pb.h"
#include "Volatility.pb.h"
#include "InterestRate.pb.h"
#include "Credit.pb.h"
#include "Destriker.pb.h"
#include "Quoter.pb.h"
#include "RequestForQuote.pb.h"

class TraderApi;
class Quoter : public Strategy {
  struct Parameter
  {
    double credit;
    double multiplier;
    double destriker;
    PricePtr price;
    TheoMatrixPtr theos;
    int32_t bid_refills;
    int32_t ask_refills;
    bool is_on;
    bool is_qr;
    OrderPtr bid;
    OrderPtr ask;
    bool canceling;
    std::string qr_id;
    Proto::InstrumentStatus status;
    int64_t auction_time;
    base::VolumeType auction_volume;
  };
  typedef std::shared_ptr<Parameter> ParameterPtr;
  typedef std::unordered_map<const Instrument*, ParameterPtr> ParameterMap;

 public:
  Quoter(const std::string &name, DeviceManager *dm);

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
  bool OnRequestForQuote(const std::shared_ptr<Proto::RequestForQuote> &msg);
  bool OnSSRate(const std::shared_ptr<Proto::SSRate> &msg);
  bool OnCredit(const std::shared_ptr<Proto::Credit> &msg);
  bool OnVolatilityCurve(const std::shared_ptr<Proto::VolatilityCurve> &msg);
  bool OnInterestRateReq(const std::shared_ptr<Proto::InterestRateReq> &msg);
  bool OnDestriker(const std::shared_ptr<Proto::Destriker> &msg);
  // bool OnQuoterSpec(const std::shared_ptr<Proto::QuoterSpec> &msg);
  bool OnStrategySwitch(const std::shared_ptr<Proto::StrategySwitch> &msg);
  bool OnStrategyOperate(const std::shared_ptr<Proto::StrategyOperate> &msg);
  bool OnInstrumentReq(const std::shared_ptr<Proto::InstrumentReq> &msg);
  void CheckForAuction();
  void CheckForQR();
  void RespondingQR(const std::shared_ptr<Proto::RequestForQuote> &rfq);

  bool Check(ParameterMap::iterator &it);
  void CalculateAndResubmit(ParameterMap::iterator &it, const char *reason);
  void ResubmitOrders(ParameterMap::iterator &it, const char *reason);
  void CancelOrders(const ParameterPtr &parameter, const char *reason);
  TraderApi *api_ = nullptr;

  std::shared_ptr<Proto::QuoterSpec> quoter_;
  // std::map<boost::gregorian::date, std::shared_ptr<MaturityParameter>> parameters_;
  ParameterMap parameters_;
  std::queue<std::tuple<std::shared_ptr<Proto::RequestForQuote>, int64_t>> pending_rfqs_;
  // std::queue<std::tuple<ParameterMap::iterator, int64_t>> active_rfqs_;

  bool quote_;
  bool amend_quote_;
  bool side_quote_;

  OrderPtr bid_;
  OrderPtr ask_;
  std::unordered_map<size_t, OrderPtr> orders_;
  std::shared_ptr<Proto::StrategyStatistic> statistic_;
};

#endif // STRATEGY_QUOTER_H
