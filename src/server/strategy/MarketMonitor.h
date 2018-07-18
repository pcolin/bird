#ifndef STRATEGY_MARKET_MONITOR_H
#define STRATEGY_MARKET_MONITOR_H

#include "base/concurrency/blockingconcurrentqueue.h"
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
  // virtual void OnLastEvent() override;

private:
  bool OnInstrumentReq(const std::shared_ptr<Proto::InstrumentReq> &req);
  bool OnPosition(const std::shared_ptr<Proto::Position> &position);
  bool OnPriceReq(const std::shared_ptr<Proto::PriceReq> &req);
  int und_price_time_;
  int opt_price_time_;

  void RunOrder();

  // std::vector<OrderPtr> orders_;
  static const size_t capacity_ = 1024;
  moodycamel::BlockingConcurrentQueue<OrderPtr> orders_;
  std::unique_ptr<std::thread> order_thread_;

  PriceMap prices_;
  // std::unique_ptr<std::thread> cash_thread_;

};

#endif
