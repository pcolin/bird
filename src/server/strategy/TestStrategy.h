#ifndef STRATEGY_TEST_STRATEGY_H
#define STRATEGY_TEST_STRATEGY_H

#include "Strategy.h"

class TraderApi;
class TestStrategy : public Strategy
{
public:
  TestStrategy(const std::string &name, DeviceManager *dm);

  virtual void OnStart() override;
  virtual void OnStop() override;

protected:
  virtual void OnPrice(const PricePtr &price) override;
  virtual void OnOrder(const OrderPtr &order) override;
  virtual void OnTrade(const TradePtr &trade) override;

private:
  bool OnStrategyStatusReq(const std::shared_ptr<proto::StrategyStatusReq> &msg);
  OrderPtr NewOrder(const Instrument *inst, Side side, base::PriceType price);
  TraderApi *api_ = nullptr;

  std::unordered_map<const Instrument*, OrderPtr> orders_;
  std::unordered_map<const Instrument*, base::PriceType> prices_;
};

#endif
