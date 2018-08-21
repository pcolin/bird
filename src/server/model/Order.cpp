#include "Order.h"
// #include "base/logger/Logging.h"
#include "Order.pb.h"
#include "base/logger/LogStream.h"

#include <map>
#include <sstream>
#include <boost/format.hpp>
#include <boost/uuid/uuid_generators.hpp>
// #include <boost/uuid/uuid_io.hpp>
// #include <boost/lexical_cast.hpp>

thread_local boost::uuids::random_generator id_generator;

Order::Order()
  : header(MsgType::Order),
    id(boost::uuids::hash_value(id_generator())), status(Proto::OrderStatus::Local)
{
  header.SetTime();
}

Order::Order(const Proto::Order &ord)
  : header(MsgType::Order),
    id(ord.id()),
    price(ord.price()),
    volume(ord.volume()),
    strategy_type(ord.strategy_type()),
    side(ord.side()),
    time_condition(ord.time_condition()),
    type(ord.type()),
    status(ord.status())
{
  header.time = ord.time();
}

void Order::ResetId()
{
  id = boost::uuids::hash_value(id_generator());
}

// std::string Order::Dump() const
// {
//   // static std::map<Side, const char*> sides = {
//   //   {Side::Buy, "Buy"}, {Side::Sell, "Sell"}, {Side::BuyCover, "BuyCover"},
//   //   {Side::BuyCoverToday, "BuyCoverToday"}, {Side::BuyCoverYesterday, "BuyCoverYesterDay"},
//   //   {Side::SellCover, "SellCover"}, {Side::SellCoverToday, "SellCoverToday"},
//   //   {Side::SellCoverYesterday, "SellCoverYesterday"} };
//   // static std::map<TimeCondition, const char*> time_conditions = {
//   //   {TimeCondition::GTD, "GTD"}, {TimeCondition::IOC, "IOC"} };
//   // static const char* time_conditions[] = { "GTD", "IOC" };
//   // static std::map<OrderStatus, const char*> statuses = {
//   //   {OrderStatus::Undefined, "Undefined"}, {OrderStatus::Local, "Local"},
//   //   {OrderStatus::Submitted, "Submitted"}, {OrderStatus::New, "New"},
//   //   {OrderStatus::PartialFilled, "PartialFilled"}, {OrderStatus::Filled, "Filled"},
//   //   {OrderStatus::PartialFilledCanceled, "PartialFilledCanceled"},
//   //   {OrderStatus::Canceled, "Canceled"}, {OrderStatus::Rejected, "Rejected"} };
//   std::stringstream ss;
//   ss << boost::format("%1% %2% %3% %4%@%5% %6% %7%") % id % instrument->Id() % Proto::Side_Name(side)
//     % volume % price % Proto::TimeCondition_Name(time_condition) % Proto::OrderStatus_Name(status);

//   if (!counter_id.empty())
//     ss << boost::format(" CounterID(%1%)") % counter_id;
//   if (!exchange_id.empty())
//     ss << boost::format(" ExchangeID(%1%)") % exchange_id;
//   if (!strategy.empty())
//     ss << boost::format(" Strategy(%1%:%2%)") % Proto::StrategyType_Name(strategy_type) % strategy;
//   if (avg_executed_price > 0)
//     ss << boost::format(" Executed(%1%@%2%)") % executed_volume % avg_executed_price;
//   if (header.interval[2] > 0)
//     ss << boost::format(" Delay(%1%,%2%,%3%,%4%)") %
//       header.time % header.interval[0] % header.interval[1] % header.interval[2];
//   if (!note.empty())
//     ss << boost::format(" Note(%1%)") % note;
//   return ss.str();
// }

std::shared_ptr<Proto::Order> Order::Serialize() const
{
  std::shared_ptr<Proto::Order> ord = Message::NewProto<Proto::Order>();
  ord->set_id(id);
  ord->set_instrument(instrument->Id());
  ord->set_counter_id(counter_id);
  ord->set_exchange_id(exchange_id);
  ord->set_note(note);
  ord->set_price(price);
  ord->set_avg_executed_price(avg_executed_price);
  ord->set_volume(volume);
  ord->set_executed_volume(executed_volume);
  ord->set_exchange(instrument->Exchange());
  ord->set_strategy_type(strategy_type);
  ord->set_side(side);
  ord->set_time_condition(time_condition);
  ord->set_type(type);
  ord->set_status(status);
  // google::protobuf::Timestamp *t = new google::protobuf::Timestamp;
  // t->set_seconds(header.time / 1000000);
  // t->set_nanos(header.time % 1000000 * 1000);
  // ord->set_allocated_time(t);
  ord->set_time(header.time);
  ord->set_latency(header.interval[2]);
  return ord;
}

void Order::Serialize(Proto::Order *order) const
{
  order->set_id(id);
  order->set_instrument(instrument->Id());
  order->set_counter_id(counter_id);
  order->set_exchange_id(exchange_id);
  order->set_note(note);
  order->set_price(price);
  order->set_avg_executed_price(avg_executed_price);
  order->set_volume(volume);
  order->set_executed_volume(executed_volume);
  order->set_exchange(instrument->Exchange());
  order->set_strategy_type(strategy_type);
  order->set_side(side);
  order->set_time_condition(time_condition);
  order->set_type(type);
  order->set_status(status);
  // google::protobuf::Timestamp *t = new google::protobuf::Timestamp;
  // t->set_seconds(header.time / 1000000);
  // t->set_nanos(header.time % 1000000 * 1000);
  // order->set_allocated_time(t);
  order->set_time(header.time);
  order->set_latency(header.interval[2]);
}

namespace base
{
LogStream& operator<<(LogStream& stream, const OrderPtr &order)
{
  if (order)
  {
    stream << boost::format("%1% %2% %3% %4%@%5% %6% %7%") % order->id % order->instrument->Id() %
      Proto::Side_Name(order->side) % order->volume % order->price %
      Proto::TimeCondition_Name(order->time_condition) % Proto::OrderStatus_Name(order->status);

    if (!order->counter_id.empty())
    {
      stream << boost::format(" CounterID(%1%)") % order->counter_id;
    }
    if (!order->exchange_id.empty())
    {
      stream << boost::format(" ExchangeID(%1%)") % order->exchange_id;
    }
    if (!order->strategy.empty())
    {
      stream << boost::format(" Strategy(%1%:%2%)") %
        Proto::StrategyType_Name(order->strategy_type) % order->strategy;
    }
    if (order->avg_executed_price > 0)
    {
      stream << boost::format(" Executed(%1%@%2%)") % order->executed_volume %
        order->avg_executed_price;
    }
    if (order->header.interval[2] > 0)
    {
      stream << boost::format(" Latency(%1%,%2%,%3%,%4%)") % order->header.time %
        order->header.interval[0] % order->header.interval[1] % order->header.interval[2];
    }
    if (!order->note.empty())
    {
      stream << boost::format(" Note(%1%)") % order->note;
    }
  }
  else
  {
    stream.append("(NULL)", 6);
  }
  return stream;
}
}
