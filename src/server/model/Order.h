#ifndef MODEL_ORDER_H
#define MODEL_ORDER_H

#include "Message.h"
#include "Instrument.h"
#include <memory>

enum class Side : int8_t
{
  Buy = 0,
  Sell = 1,
  BuyCover,
  BuyCoverToday,
  BuyCoverYesterday,
  SellCover,
  SellCoverToday,
  SellCoverYesterday,
};

enum class TimeCondition : int8_t
{
  GTD = 0,
  IOC,
};

enum class OrderType : int8_t
{
  Limit = 0,
  Market,
};

enum class OrderStatus : int8_t
{
  Undefined = 0,
  Local,
  Submitted,
  New,
  PartialFilled,
  Filled,
  PartialFilledCanceled,
  Canceled,
  Rejected,
};

struct Order
{
  Order();
  bool IsBid() const
  {
    return side == Side::Buy || side == Side::BuyCover ||
      side == Side::BuyCoverToday || side == Side::BuyCoverYesterday;
  }

  bool IsOpen() const
  {
    return side == Side::Buy || side == Side::Sell;
  }

  bool IsInactive() const
  {
    return status >= OrderStatus::Filled;
  }

  std::string Dump() const;

  MsgHeader header;
  const Instrument *instrument;
  size_t id;
  std::string counter_id;
  std::string exchange_id;
  std::string strategy;
  std::string note;
  base::PriceType price;
  base::PriceType avg_executed_price;
  base::VolumeType volume;
  base::VolumeType executed_volume;
  Side side;
  TimeCondition time_condition;
  OrderType type;
  OrderStatus status;
};

typedef std::shared_ptr<Order> OrderPtr;

enum class OrderAction : int8_t
{
  New = 0,
  Pull,
  Amend,
};

struct OrderRequest
{
  OrderRequest(const OrderPtr &ord, OrderAction act)
    : order(ord), action(act)
  {}

  OrderPtr order;
  OrderAction action;
};

typedef std::shared_ptr<OrderRequest> OrderRequestPtr;

struct QuoteRequest
{
  QuoteRequest(const OrderPtr &b, const OrderPtr &a, OrderAction act)
    : bid(b), ask(a), action(act)
  {}

  OrderPtr bid;
  OrderPtr ask;
  OrderAction action;
};

typedef std::shared_ptr<QuoteRequest> QuoteRequestPtr;

#endif
