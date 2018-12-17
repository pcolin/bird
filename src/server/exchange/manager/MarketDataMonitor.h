#ifndef EXCHANGE_MARKET_DATA_MONITOR_H
#define EXCHANGE_MARKET_DATA_MONITOR_H

#include <atomic>
#include <thread>
#include "model/Price.h"

class MarketDataMonitor {
public:
  void Start();
  void Stop();
  void OnPrice(const PricePtr& price);

private:
  void RunTimer();

  std::atomic<bool> running_;
  std::atomic<int> und_price_time_;
  std::atomic<int> opt_price_time_;
  const Instrument* instrument_;
  std::thread thread_;
};

#endif
