#ifndef STRATEGY_QUOTER_H
#define STRATEGY_QUOTER_H

#include "Strategy.h"
#include "SSRate.pb.h"
#include "Credit.pb.h"
#include "Quoter.pb.h"

class TraderApi;
class Quoter : public Strategy
{
  // typedef std::tuple<OrderPtr, OrderPtr, bool> OrderTuple;
  typedef std::tuple<OrderPtr, bool> OrderTuple;
  struct Parameter
  {
    double credit;
    double destriker;
    PricePtr price;
    TheoMatrixPtr theos;
    TheoData theo;
    int position;
    int32_t refill_times;
    bool is_bid;
    bool is_ask;
    bool is_qr;
    OrderPtr bid;
    OrderPtr ask;
    bool bid_canceling;
    bool ask_canceling;
    // std::list<OrderTuple> orders;
  };
  typedef std::shared_ptr<Parameter> ParameterPtr;

  struct MaturityParameter
  {
    double basis;
    double price;
    double multiplier;
    std::unordered_map<const Instrument*, ParameterPtr> parameters;
  };

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
  bool OnSSRate(const std::shared_ptr<Proto::SSRate> &msg);
  bool OnCredit(const std::shared_ptr<Proto::Credit> &msg);
  bool OnQuoterSpec(const std::shared_ptr<Proto::QuoterSpec> &msg);
  bool OnStrategySwitch(const std::shared_ptr<Proto::StrategySwitch> &msg);
  bool OnStrategyOperate(const std::shared_ptr<Proto::StrategyOperate> &msg);
  void PublishStatistic();

  bool Check(const Instrument *inst, double multiplier, ParameterPtr &parameter);
  void Requote(const Instrument *inst, double spot, double multiplier, ParameterPtr &parameter);
  bool GetBidPrice(const Instrument *inst, double theo, double multiplier, double credit,
      double &price);
  bool GetAskPrice(const Instrument *inst, double theo, double multiplier, double credit,
      double &price);
  // bool GetBidAskPrice(const Instrument *inst, double theo, double multiplier, double credit,
  //     double &bid_price, double &ask_price);
  bool IsBidAdjusted(const OrderPtr &bid, double theo, double multiplier, double credit);
  bool IsAskAdjusted(const OrderPtr &ask, double theo, double multiplier, double credit);

  void ResubmitBidOrder(const Instrument *inst, double multiplier, ParameterPtr &parameter);
  void SubmitBidOrder(const Instrument *inst, double multiplier, ParameterPtr &parameter);
  void ResubmitAskOrder(const Instrument *inst, double multiplier, ParameterPtr &parameter);
  void SubmitAskOrder(const Instrument *inst, double multiplier, ParameterPtr &parameter);
  void SubmitOrders(const Instrument *inst, double theo, double multiplier, ParameterPtr &parameter);
  void CancelOrders(const std::shared_ptr<Parameter> &parameter);
  OrderPtr NewOrder(const Instrument *inst, const OrderPtr &order, base::PriceType price);
  TraderApi *api_ = nullptr;

  std::shared_ptr<Proto::QuoterSpec> quoter_;
  std::map<boost::gregorian::date, std::shared_ptr<MaturityParameter>> parameters_;

  bool quote_;
  bool amend_quote_;
  bool side_quote_;

  OrderPtr bid_;
  OrderPtr ask_;
  std::unordered_set<size_t> order_ids_;
  int32_t orders_;
  int32_t trades_;
  double delta_;
};

#endif
