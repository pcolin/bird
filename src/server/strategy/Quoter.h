#ifndef STRATEGY_QUOTER_H
#define STRATEGY_QUOTER_H

#include "Strategy.h"
#include "Credit.pb.h"
#include "Quoter.pb.h"

class TraderApi;
class Quoter : public Strategy
{
  struct Parameter
  {
    PricePtr price;
    std::list<std::pair<OrderPtr, OrderPtr>> orders;
    double credit;
    double multiplier;
    int32_t refill_times;
    bool is_bid;
    bool is_ask;
    bool is_qr;
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
  bool OnCredit(const std::shared_ptr<Proto::Credit> &msg);
  bool OnQuoterSpec(const std::shared_ptr<Proto::QuoterSpec> &msg);
  bool OnStrategySwitch(const std::shared_ptr<Proto::StrategySwitch> &msg);
  bool OnStrategyOperate(const std::shared_ptr<Proto::StrategyOperate> &msg);

  OrderPtr NewOrder(const Instrument *inst, Proto::Side side, base::PriceType price);
  TraderApi *api_ = nullptr;

  TheoMatrixPtr theo_;
  std::shared_ptr<Proto::QuoterSpec> quoter_;
  std::unordered_map<const Instrument*, std::shared_ptr<Parameter>> parameters_;

  std::unordered_set<size_t> order_ids_;
  int32_t trades_;
};

#endif
