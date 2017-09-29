#include "MarketMonitor.h"
#include "base/logger/Logging.h"
#include "model/OrderManager.h"
#include "model/PositionManager.h"

#include <boost/format.hpp>
#include <sys/time.h>

MarketMonitor::MarketMonitor(const std::string &name, DeviceManager *dm)
  : Strategy(name, dm)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  und_price_time_ = tv.tv_sec;
  opt_price_time_ = tv.tv_sec;

  dispatcher_.RegisterCallback<PROTO::Position>(
      std::bind(&MarketMonitor::OnMessage, this, std::placeholders::_1));
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
    opt_price_time_ = now;
  }
  interval = now - und_price_time_;
  if (interval >= max_interval)
  {
    LOG_ERR << boost::format("%1% : Underlying price feed timeout for %2%s") %
      dm_->GetUnderlying()->Id() % interval;
    und_price_time_ = now;
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
  // orders_.push_back(order);
  OrderManager::GetInstance()->OnOrder(order);
  /// to be done... publish
}

void MarketMonitor::OnTrade(const TradePtr &trade)
{
  // LOG_INF << "OnTrade : " << trade->DebugString();
}

void MarketMonitor::OnLastEvent()
{
  if (orders_.size() > 0)
  {
    LOG_INF << boost::format("Update %1% orders") % orders_.size();
    OrderManager::GetInstance()->OnOrder(orders_);
    orders_.clear();
  }
}

void MarketMonitor::OnMessage(const std::shared_ptr<PROTO::Position> &position)
{
  LOG_INF << "Position: " << position->ShortDebugString();
  PositionManager::GetInstance()->UpdatePosition(position);
}
