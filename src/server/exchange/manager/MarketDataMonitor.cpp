#include "MarketDataMonitor.h"
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"
#include "model/ProductManager.h"
#include "boost/format.hpp"

void MarketDataMonitor::Start() {
  instrument_ = ProductManager::GetInstance()->FindInstrument(
      [](const auto& inst){ return true; });
  if (instrument_) {
    bool expected = false;
    if (running_.compare_exchange_strong(expected, true)) {
      LOG_INF << Proto::Exchange_Name(instrument_->Exchange())
              << " market data monitor started...";
      struct timeval tv;
      gettimeofday(&tv, NULL);
      und_price_time_ = tv.tv_sec;
      opt_price_time_ = tv.tv_sec;
      thread_ = std::thread(&MarketDataMonitor::RunTimer, this);
    }
  } else {
    LOG_WAN << "can't get any instrument, monitor isn't started";
  }
}

void MarketDataMonitor::Stop() {
  bool expected = true;
  if (running_.compare_exchange_strong(expected, false)) {
    thread_.join();
    LOG_INF << Proto::Exchange_Name(instrument_->Exchange())
            << " market data monitor stopped";
  }
}

void MarketDataMonitor::OnPrice(const PricePtr& price)
{
  assert(price->instrument);
  struct timeval tv;
  gettimeofday(&tv, NULL);
  if (price->instrument->Type() == Proto::InstrumentType::Option) {
    opt_price_time_ = tv.tv_sec;
  } else {
    und_price_time_ = tv.tv_sec;
  }
}

void MarketDataMonitor::RunTimer() {
  const int TIME_OUT = EnvConfig::GetInstance()->GetInt32(EnvVar::PRICE_TIMEOUT, 10);
  while (running_) {
    std::this_thread::sleep_for(std::chrono::seconds(TIME_OUT));
    if (instrument_->Status() == Proto::InstrumentStatus::Trading) {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      int interval = tv.tv_sec - opt_price_time_;
      if (interval > TIME_OUT) {
        LOG_ERR << boost::format("%1%: option price feed timeout for %2%s") %
                   Proto::Exchange_Name(instrument_->Exchange()) % interval;
      }
      interval = tv.tv_sec - und_price_time_;
      if (interval > TIME_OUT) {
        LOG_ERR << boost::format("%1%: underlying price feed timeout for %2%s") %
                   Proto::Exchange_Name(instrument_->Exchange()) % interval;
      }
    }
  }
}
