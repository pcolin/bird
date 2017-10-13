#include "TestStrategy.h"
#include "base/logger/Logging.h"
#include "exchange/manager/ExchangeManager.h"

#include <boost/format.hpp>

TestStrategy::TestStrategy(const std::string &name, DeviceManager *dm)
  : Strategy(name, dm)
{
  dispatcher_.RegisterCallback<proto::StrategyStatusReq>(
      std::bind(&TestStrategy::OnStrategyStatusReq, this, std::placeholders::_1));
}

void TestStrategy::OnStart()
{
  LOG_INF << "OnStart";
  api_ = ExchangeManager::GetInstance()->GetTraderApi();
  assert(api_);
}

void TestStrategy::OnStop()
{
  LOG_INF << "OnStop";
  for(auto &it : orders_)
  {
    if (!it.second->IsInactive())
    {
      LOG_INF << boost::format("Pull order(stop): %1%") % it.second->Dump();
      api_->Pull(it.second);
    }
  }
}

void TestStrategy::OnPrice(const PricePtr &price)
{
  LOG_INF << "OnPrice : " << price->Dump();
  if (price->instrument->Type() == InstrumentType::Option)
  {
    auto it = orders_.find(price->instrument);
    if (it != orders_.end())
    {
      if (it->second && std::abs(price->last - it->second->price) >= 5)
      {
        api_->Pull(it->second);
        LOG_INF << boost::format("Pull order(pull to resubmit): %1%") % it->second->Dump();
      }
    }
    else
    {
      auto ord = NewOrder(price->instrument, Side::Buy, price->last);
      api_->New(ord);
      orders_[price->instrument] = ord;
    }
  }
  prices_[price->instrument] = price->last;
}

void TestStrategy::OnOrder(const OrderPtr &order)
{
  LOG_INF << "OnOrder : " << order->Dump();
  if (order->strategy == name_)
  {
    if (order->IsInactive())
    {
      /// new order
      // auto ord = NewOrder(order->instrument, Side::Buy, prices_[order->instrument]);
      // api_->New(ord);
      // orders_[order->instrument] = ord;
      orders_[order->instrument] = order;
    }
    else
    {
      orders_[order->instrument] = order;
    }
  }
}

void TestStrategy::OnTrade(const TradePtr &trade)
{
  LOG_INF << "OnTrade: " << trade->Dump();
}

bool TestStrategy::OnStrategyStatusReq(const std::shared_ptr<proto::StrategyStatusReq> &msg)
{
  for (auto &s : msg->statuses())
  {
    if (s.name() == Name() && s.status() == proto::StrategyStatus::Play)
    {
      // LOG_PUB << boost::format("%1% played %2%") % msg->user() % s.name();
    }
  }
}

OrderPtr TestStrategy::NewOrder(const Instrument *inst, Side side, base::PriceType price)
{
  auto ord = Message::NewOrder();
  ord->instrument = inst;
  ord->strategy = name_;
  ord->price = price;
  ord->volume = 10;
  ord->side = side;
  ord->time_condition = TimeCondition::GTD;
  ord->type = OrderType::Limit;
  ord->status = OrderStatus::Local;
  return ord;
}
