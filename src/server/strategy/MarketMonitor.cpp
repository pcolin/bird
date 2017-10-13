#include "MarketMonitor.h"
#include "Price.pb.h"
#include "Order.pb.h"
#include "Trade.pb.h"
#include "base/logger/Logging.h"
#include "model/OrderManager.h"
#include "model/PositionManager.h"
#include "model/Middleware.h"

#include <boost/format.hpp>
#include <sys/time.h>

MarketMonitor::MarketMonitor(const std::string &name, DeviceManager *dm)
  : Strategy(name, dm)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  und_price_time_ = tv.tv_sec;
  opt_price_time_ = tv.tv_sec;

  dispatcher_.RegisterCallback<proto::Position>(
      std::bind(&MarketMonitor::OnPosition, this, std::placeholders::_1));
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
  Middleware::GetInstance()->Publish(price->Serialize());
}

void MarketMonitor::OnOrder(const OrderPtr &order)
{
  // LOG_INF << "OnOrder : " << order->DebugString();
  OrderManager::GetInstance()->OnOrder(order);
  Middleware::GetInstance()->Publish(order->Serialize());
}

void MarketMonitor::OnTrade(const TradePtr &trade)
{
  // LOG_INF << "OnTrade : " << trade->DebugString();
  Middleware::GetInstance()->Publish(trade->Serialize());
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

bool MarketMonitor::OnPosition(const std::shared_ptr<proto::Position> &position)
{
  LOG_INF << "Position: " << position->ShortDebugString();
  PositionManager::GetInstance()->UpdatePosition(position);
  return true;
}
