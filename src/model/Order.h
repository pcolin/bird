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

  MsgHeader header;
  const Instrument *instrument;
  const size_t local_id;
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

#endif
