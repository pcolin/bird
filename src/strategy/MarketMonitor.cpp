#include "MarketMonitor.h"
#include "base/logger/Logging.h"

#include <boost/format.hpp>
#include <sys/time.h>

MarketMonitor::MarketMonitor(const std::string &name, DeviceManager *dm)
  : Strategy(name, dm)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  und_price_time_ = tv.tv_sec;
  opt_price_time_ = tv.tv_sec;
}

void MarketMonitor::OnStart()
{
  LOG_INF << "OnStart";
}

void MarketMonitor::OnStop()
{
  LOG_INF << "OnStop";
}

void MarketMonitor::OnPrice(const PricePtr &price)
{
  LOG_INF << "OnPrice : " << price->Dump();
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int now = tv.tv_sec;
  const int max_interval = 15;
  int interval = now - opt_price_time_;
  if (interval >= max_interval)
  {
    LOG_ERR << boost::format("%1% : Option price feed timeout for %2%s") %
      dm_->GetUnderlying()->Id() % interval;
  }
  interval = now - und_price_time_;
  if (interval >= max_interval)
  {
    LOG_ERR << boost::format("%1% : Underlying price feed timeout for %2%s") %
      dm_->GetUnderlying()->Id() % interval;
  }
  if (price->instrument->Type() == InstrumentType::Option)
  {
    opt_price_time_ = now;
  }
  else
  {
    und_price_time_ = now;
  }
}

void MarketMonitor::OnOrder(const OrderPtr &order)
{
  // LOG_INF << "OnOrder : " << order->DebugString();
}

void MarketMonitor::OnTrade(const TradePtr &trade)
{
  // LOG_INF << "OnTrade : " << trade->DebugString();
}
