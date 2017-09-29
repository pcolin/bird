#include "Order.h"
// #include "base/logger/Logging.h"

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
  static std::map<TimeCondition, const char*> time_conditions = {
    {TimeCondition::GTD, "GTD"}, {TimeCondition::IOC, "IOC"} };
  static std::map<OrderStatus, const char*> statuses = {
    {OrderStatus::Undefined, "Undefined"}, {OrderStatus::Local, "Local"},
    {OrderStatus::Submitted, "Submitted"}, {OrderStatus::New, "New"},
    {OrderStatus::PartialFilled, "PartialFilled"}, {OrderStatus::Filled, "Filled"},
    {OrderStatus::PartialFilledCanceled, "PartialFilledCanceled"},
    {OrderStatus::Canceled, "Canceled"}, {OrderStatus::Rejected, "Rejected"} };
  std::stringstream ss;
  ss << boost::format("%1% %2% %3% %4%@%5% %6% %7%") % id % instrument->Id() % sides[side] % volume %
    price % time_conditions[time_condition] % statuses[status];

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
