#ifndef STRATEGY_MARKET_MONITOR_H
#define STRATEGY_MARKET_MONITOR_H

#include "Strategy.h"
#include "Position.pb.h"

class MarketMonitor : public Strategy
{
public:
  MarketMonitor(const std::string &name, DeviceManager *dm);

  virtual void OnStart() override;
  virtual void OnStop() override;

protected:
  virtual void OnPrice(const PricePtr &price) override;
  virtual void OnOrder(const OrderPtr &order) override;
  virtual void OnTrade(const TradePtr &trade) override;

private:
  void OnMessage(const std::shared_ptr<PROTO::Position> &position);
  int und_price_time_;
  int opt_price_time_;

};

#endif