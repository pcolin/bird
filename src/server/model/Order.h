#ifndef MODEL_ORDER_H
#define MODEL_ORDER_H

#include <memory>
#include "Message.h"
#include "Instrument.h"
#include "Order.pb.h"

struct Order {
  Order();
  Order(const Proto::Order &ord);

  void ResetId();

  bool IsBid() const {
    return side == Proto::Side::Buy || side == Proto::Side::BuyCover ||
           side == Proto::Side::BuyCoverToday || side == Proto::Side::BuyCoverYesterday;
  }

  bool IsOpen() const { return side == Proto::Side::Buy || side == Proto::Side::Sell; }

  bool IsCloseToday() const {
    return side == Proto::Side::BuyCoverToday || side == Proto::Side::SellCoverToday;
  }

  bool IsInactive() const { return status >= Proto::OrderStatus::Filled; }

  // std::string Dump() const;
  std::shared_ptr<Proto::Order> Serialize() const;
  void Serialize(Proto::Order *order) const;
  // static OrderPtr Deserilize(const )

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
  Proto::StrategyType strategy_type;
  Proto::Side side;
  Proto::TimeCondition time_condition;
  Proto::OrderType type;
  Proto::OrderStatus status;
};

typedef std::shared_ptr<Order> OrderPtr;

struct OrderRequest {
  OrderRequest(const OrderPtr &ord, Proto::OrderAction act)
      : order(ord), action(act)
  {}

  OrderPtr order;
  Proto::OrderAction action;
};

typedef std::shared_ptr<OrderRequest> OrderRequestPtr;

struct QuoteRequest {
  QuoteRequest(const OrderPtr &b, const OrderPtr &a, Proto::OrderAction act)
      : bid(b), ask(a), action(act)
  {}

  OrderPtr bid;
  OrderPtr ask;
  Proto::OrderAction action;
};

typedef std::shared_ptr<QuoteRequest> QuoteRequestPtr;

namespace base {
  
class LogStream;
LogStream& operator<<(LogStream& stream, const OrderPtr &order);

} // namespace base

#endif // MODEL_ORDER_H
