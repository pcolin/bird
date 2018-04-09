#ifndef STRATEGY_MARKET_MONITOR_H
#define STRATEGY_MARKET_MONITOR_H

#include "Strategy.h"
#include "Position.pb.h"

class MarketMonitor : public Strategy
{
  typedef std::unordered_map<const Instrument*, std::shared_ptr<Proto::Price>> PriceMap; 
public:
  MarketMonitor(const std::string &name, DeviceManager *dm);

  virtual void OnStart() override;
  virtual void OnStop() override;

protected:
  virtual void OnPrice(const PricePtr &price) override;
  virtual void OnOrder(const OrderPtr &order) override;
  virtual void OnTrade(const TradePtr &trade) override;
  virtual void OnLastEvent() override;

private:
  bool OnPosition(const std::shared_ptr<Proto::Position> &position);
  bool OnPriceReq(const std::shared_ptr<Proto::PriceReq> &req);
  int und_price_time_;
  int opt_price_time_;

  std::vector<OrderPtr> orders_;

  PriceMap prices_;
  // std::unique_ptr<std::thread> cash_thread_;

};

#endif
