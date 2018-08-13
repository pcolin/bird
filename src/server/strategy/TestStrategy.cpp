#include "TestStrategy.h"
#include "base/logger/Logging.h"
#include "exchange/manager/ExchangeManager.h"

// #include "strategy.pb.h"

#include <boost/format.hpp>

TestStrategy::TestStrategy(const std::string &name, DeviceManager *dm)
  : Strategy(name, dm)
{
  dispatcher_.RegisterCallback<Proto::StrategyOperate>(
      std::bind(&TestStrategy::OnStrategyOperate, this, std::placeholders::_1));
}

void TestStrategy::OnStart()
{
  LOG_INF << "OnStart";
  api_ = ExchangeManager::GetInstance()->GetTraderApi();
  assert(api_);
  orders_.clear();
}

void TestStrategy::OnStop()
{
  LOG_INF << "OnStop";
  for(auto &it : orders_)
  {
    if (!it.second->IsInactive())
    {
      LOG_INF << "Cancel order(stop): " << it.second;
      api_->Cancel(it.second);
    }
  }
}

void TestStrategy::OnPrice(const PricePtr &price)
{
  LOG_DBG << price;
  if (price->instrument->Type() == Proto::InstrumentType::Option)
  {
    auto it = orders_.find(price->instrument);
    if (it != orders_.end())
    {
      if (it->second && std::abs(price->last - it->second->price) >= 5)
      {
        api_->Cancel(it->second);
        LOG_INF << "Cancel order(pull to resubmit): " << it->second;
      }
    }
    else
    {
      auto ord = NewOrder(price->instrument, Proto::Side::Buy, price->last);
      api_->Submit(ord);
      orders_[price->instrument] = ord;
    }
  }
  prices_[price->instrument] = price->last;
}

void TestStrategy::OnOrder(const OrderPtr &order)
{
  LOG_DBG << order;
  if (order->strategy == name_)
  {
    if (order->IsInactive())
    {
      /// new order
      // auto ord = NewOrder(order->instrument, Side::Buy, prices_[order->instrument]);
      // api_->Submit(ord);
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
  LOG_DBG << trade;
}

bool TestStrategy::OnStrategyOperate(const std::shared_ptr<Proto::StrategyOperate> &msg)
{
  if (msg->name() == Name() && msg->operate() == Proto::StrategyOperation::Start)
  {
    // LOG_PUB << boost::format("%1% played %2%") % msg->user() % s.name();
  }
}

OrderPtr TestStrategy::NewOrder(const Instrument *inst, Proto::Side side, base::PriceType price)
{
  auto ord = Message::NewOrder();
  ord->instrument = inst;
  ord->strategy = name_;
  ord->price = price;
  ord->volume = 10;
  ord->side = side;
  ord->time_condition = Proto::TimeCondition::GTD;
  ord->type = Proto::OrderType::Limit;
  ord->status = Proto::OrderStatus::Local;
  return ord;
}
