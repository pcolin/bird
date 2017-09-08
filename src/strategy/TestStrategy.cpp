#include "TestStrategy.h"
#include "base/logger/Logging.h"

TestStrategy::TestStrategy(DeviceManager *dm)
  : Strategy(dm)
{}

void TestStrategy::OnStart()
{
  LOG_INF << "OnStart";
}

void TestStrategy::OnStop()
{
  LOG_INF << "OnStop";
}

void TestStrategy::OnPrice(const PricePtr &price)
{
  LOG_INF << "OnPrice : " << price->DebugString();
}

void TestStrategy::OnOrder(const OrderPtr &order)
{
  // LOG_INF << "OnOrder : " << order->DebugString();
}

void TestStrategy::OnTrade(const TradePtr &trade)
{
  // LOG_INF << "OnTrade : " << trade->DebugString();
}
