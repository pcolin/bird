#ifndef STRATEGY_MARKET_MONITOR_H
#define STRATEGY_MARKET_MONITOR_H

#include "Strategy.h"

class MarketMonitor : public Strategy
{
public:
  MarketMonitor(DeviceManager *dm);

  virtual void OnStart() override;
  virtual void OnStop() override;

protected:
  virtual void OnPrice(const PricePtr &price) override;
  virtual void OnOrder(const OrderPtr &order) override;
  virtual void OnTrade(const TradePtr &trade) override;

private:
  int und_price_time_;
  int opt_price_time_;

};

#endif
