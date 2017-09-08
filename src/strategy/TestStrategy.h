#ifndef STRATEGY_TEST_STRATEGY_H
#define STRATEGY_TEST_STRATEGY_H

#include "Strategy.h"

class TestStrategy : public Strategy
{
public:
  TestStrategy(DeviceManager *dm);

  virtual void OnStart() override;
  virtual void OnStop() override;

protected:
  virtual void OnPrice(const PricePtr &price) override;
  virtual void OnOrder(const OrderPtr &order) override;
  virtual void OnTrade(const TradePtr &trade) override;
};

#endif
