#include "Order.h"
// #include "base/logger/Logging.h"
#include "Order.pb.h"

#include <map>
#include <sstream>
#include <boost/format.hpp>
#include <boost/uuid/uuid_generators.hpp>
// #include <boost/uuid/uuid_io.hpp>
// #include <boost/lexical_cast.hpp>

thread_local boost::uuids::random_generator id_generator;

Order::Order()
  : header(MsgType::Order),
  id(boost::uuids::hash_value(id_generator()))
{
  header.SetTime();
}

std::string Order::Dump() const
{
  static std::map<Side, const char*> sides = {
    {Side::Buy, "Buy"}, {Side::Sell, "Sell"}, {Side::BuyCover, "BuyCover"},
    {Side::BuyCoverToday, "BuyCoverToday"}, {Side::BuyCoverYesterday, "BuyCoverYesterDay"},
    {Side::SellCover, "SellCover"}, {Side::SellCoverToday, "SellCoverToday"},
    {Side::SellCoverYesterday, "SellCoverYesterday"} };
  // static std::map<TimeCondition, const char*> time_conditions = {
  //   {TimeCondition::GTD, "GTD"}, {TimeCondition::IOC, "IOC"} };
  static const char* time_conditions[] = { "GTD", "IOC" };
  static std::map<OrderStatus, const char*> statuses = {
    {OrderStatus::Undefined, "Undefined"}, {OrderStatus::Local, "Local"},
    {OrderStatus::Submitted, "Submitted"}, {OrderStatus::New, "New"},
    {OrderStatus::PartialFilled, "PartialFilled"}, {OrderStatus::Filled, "Filled"},
    {OrderStatus::PartialFilledCanceled, "PartialFilledCanceled"},
    {OrderStatus::Canceled, "Canceled"}, {OrderStatus::Rejected, "Rejected"} };
  std::stringstream ss;
  ss << boost::format("%1% %2% %3% %4%@%5% %6% %7%") % id % instrument->Id() % sides[side] % volume %
    price % time_conditions[static_cast<int>(time_condition)] % statuses[status];

  if (!counter_id.empty())
    ss << boost::format(" CounterID(%1%)") % counter_id;
  if (!exchange_id.empty())
    ss << boost::format(" ExchangeID(%1%)") % exchange_id;
  if (!strategy.empty())
    ss << boost::format(" Strategy(%1%)") % strategy;
  if (avg_executed_price > 0)
    ss << boost::format(" Executed(%1%@%2%)") % executed_volume % avg_executed_price;
  if (header.interval[2] > 0)
    ss << boost::format(" Delay(%1%,%2%,%3%,%4%)") %
      header.time % header.interval[0] % header.interval[1] % header.interval[2];
  if (!note.empty())
    ss << boost::format(" Note(%1%)") % note;
  return ss.str();
}

std::shared_ptr<proto::Order> Order::Serialize() const
{
  std::shared_ptr<proto::Order> ord = Message::NewProto<proto::Order>();
  ord->set_id(id);
  ord->set_instrument(instrument->Id());
  ord->set_counter_id(counter_id);
  ord->set_exchange_id(exchange_id);
  ord->set_note(note);
  ord->set_price(price);
  ord->set_avg_executed_price(avg_executed_price);
  ord->set_volume(volume);
  ord->set_executed_volume(executed_volume);
  // static proto::Side sides[] = { proto::Buy, proto::Sell, proto::BuyCover };
  // ord->set_side(sides[static_cast<int>(side)]);
  ord->set_side(static_cast<proto::Side>(side));
  ord->set_time_condition(static_cast<proto::TimeCondition>(time_condition));
  ord->set_type(static_cast<proto::OrderType>(type));
  ord->set_status(static_cast<proto::OrderStatus>(status));
  // google::protobuf::Timestamp *t = new google::protobuf::Timestamp;
  // t->set_seconds(header.time / 1000000);
  // t->set_nanos(header.time % 1000000 * 1000);
  // ord->set_allocated_time(t);
  ord->set_time(header.time);
  ord->set_latency(header.interval[2]);
  return ord;
}
