#include "Quoter.h"
#include "ClusterManager.h"
#include "base/logger/Logging.h"
#include "exchange/manager/ExchangeManager.h"
#include "model/ProductManager.h"

#include <future>
#include <boost/format.hpp>

Quoter::Quoter(const std::string &name, DeviceManager *dm)
  : Strategy(name, dm)
{
  dispatcher_.RegisterCallback<Proto::Credit>(
      std::bind(&Quoter::OnCredit, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::QuoterSpec>(
      std::bind(&Quoter::OnQuoterSpec, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::StrategySwitch>(
      std::bind(&Quoter::OnStrategySwitch, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::StrategyStatusReq>(
      std::bind(&Quoter::OnStrategyStatusReq, this, std::placeholders::_1));
}

void Quoter::OnStart()
{
  LOG_INF << "OnStart";
  api_ = ExchangeManager::GetInstance()->GetTraderApi();
  assert(api_);

  quoter_ = ClusterManager::GetInstance()->FindQuoter(name_);
  if (quoter_)
  {
    parameters_.clear();
    // for (auto &r : quoter_->records())
    // {
    //   auto *inst = ProductManager::GetInstance()->FindId(r.instrument());
    //   if (inst)
    //   {
    //     auto param = std::make_shared<Parameter>();
    //     param->record.CopyFrom(r);
    //   }
    // }
    order_ids_.clear();
    trades_ = 0;
  }
  else
  {
    LOG_ERR << "Quoter " << name_ << " isn't existed";
  }
}

void Quoter::OnStop()
{
  LOG_INF << "OnStop";
  for (auto &r : parameters_)
  {
    for (auto &p : r.second->orders)
    {
      // LOG_INF << boost::format("Pull order(stop): %1%") % it.second->Dump();
      api_->Pull(p.first, p.second);
    }
  }
}

void Quoter::OnPrice(const PricePtr &price)
{
  LOG_INF << "OnPrice : " << price->Dump();
  // if (price->instrument->Type() == Proto::InstrumentType::Option)
  // {
  //   auto it = orders_.find(price->instrument);
  //   if (it != orders_.end())
  //   {
  //     if (it->second && std::abs(price->last - it->second->price) >= 5)
  //     {
  //       api_->Pull(it->second);
  //       LOG_INF << boost::format("Pull order(pull to resubmit): %1%") % it->second->Dump();
  //     }
  //   }
  //   else
  //   {
  //     auto ord = NewOrder(price->instrument, Side::Buy, price->last);
  //     api_->New(ord);
  //     orders_[price->instrument] = ord;
  //   }
  // }
}

void Quoter::OnTheoMatrix(const TheoMatrixPtr &theo)
{
  LOG_INF << "OnTheoMatrix: " << theo->Dump();
}

void Quoter::OnOrder(const OrderPtr &order)
{
  LOG_INF << "OnOrder : " << order->Dump();
  if (order->strategy == name_)
  {
    auto it = parameters_.find(order->instrument);
    if (it != parameters_.end())
    {
      if (order->IsInactive())
      {
        if (order->IsBid())
        {
          for (auto &r : it->second->orders)
          {
            if (r.first && r.first->id == order->id)
            {
              r.first.reset();
              if (!r.second)
              {
                /// to be done ... new quote
              }
            }
          }
        }
        else
        {
          for (auto &r : it->second->orders)
          {
            if (r.second && r.second->id == order->id)
            {
              r.second.reset();
              if (!r.first)
              {
                /// to be done ... new quote
              }
            }
          }
        }
      }
      else
      {
        if (order->IsBid())
        {
          for (auto &r : it->second->orders)
          {
            if (r.first && r.first->id == order->id)
            {
              r.first = order;
            }
          }
        }
        else
        {
          for (auto &r : it->second->orders)
          {
            if (r.second && r.second->id == order->id)
            {
              r.second = order;
            }
          }
        }
      }
    }
  }
}

void Quoter::OnTrade(const TradePtr &trade)
{
  LOG_INF << "OnTrade: " << trade->Dump();
  if (order_ids_.find(trade->order_id) != order_ids_.end())
  {
    ++trades_;
    if (trades_ >= quoter_->trade_limit())
    {
      std::async(std::launch::async, &DeviceManager::Stop, dm_, name_, "trade limit broken");
    }
  }
}

bool Quoter::OnCredit(const std::shared_ptr<Proto::Credit> &msg)
{
  LOG_INF << "Credit: " << msg->ShortDebugString();
}

bool Quoter::OnQuoterSpec(const std::shared_ptr<Proto::QuoterSpec> &msg)
{
  LOG_INF << "QuoterSpec: " << msg->ShortDebugString();
}

bool Quoter::OnStrategySwitch(const std::shared_ptr<Proto::StrategySwitch> &msg)
{
  LOG_INF << "StrategySwitch: " << msg->ShortDebugString();
  if (msg->strategy() == Proto::StrategyType::Quoter)
  {
    auto *inst = ProductManager::GetInstance()->FindId(msg->option());
    if (inst)
    {
      auto it = parameters_.find(inst);
      if (it != parameters_.end())
      {
        bool bid = msg->is_bid(), ask = msg->is_ask();
        if (it->second->is_bid && it->second->is_ask)
        {
          if (!bid && !ask)
          {
            for (auto &p : it->second->orders)
            {
              api_->Pull(p.first, p.second);
            }
          }
          else if (!bid)
          {
            for (auto &p : it->second->orders)
            {
              api_->Pull(p.first);
            }
          }
        }
        else if (it->second->is_bid)
        {

        }
        else
        {

        }
      }
    }
  }
}

bool Quoter::OnStrategyStatusReq(const std::shared_ptr<Proto::StrategyStatusReq> &msg)
{
  for (auto &s : msg->statuses())
  {
    if (s.name() == Name() && s.status() == Proto::StrategyStatus::Play)
    {
      // LOG_PUB << boost::format("%1% played %2%") % msg->user() % s.name();
    }
  }
}

OrderPtr Quoter::NewOrder(const Instrument *inst, Side side, base::PriceType price)
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
