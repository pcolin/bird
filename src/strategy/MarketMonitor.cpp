#include "MarketMonitor.h"
#include "base/logger/Logging.h"

#include <boost/format.hpp>
#include <sys/time.h>

MarketMonitor::MarketMonitor(DeviceManager *dm)
  : Strategy(dm)
{}

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
  LOG_INF << "OnPrice : " << price->DebugString();
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int now = tv.tv_sec;
  const int max_interval = 15;
  if (price->instrument->Type() == InstrumentType::Option)
  {
    int interval = now - opt_price_time_;
    if (interval >= max_interval)
    {
      LOG_ERR << boost::format("Option price feed timeout for %1%s") % interval;
    }
    opt_price_time_ = now;
  }
  else
  {
    int interval = now - und_price_time_;
    if (interval >= max_interval)
    {
      LOG_ERR << boost::format("Underlying price feed timeout for %1%s") % interval;
    }
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
