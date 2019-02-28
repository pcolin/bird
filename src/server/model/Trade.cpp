#include "Trade.h"
#include <map>
#include "boost/format.hpp"
#include "Trade.pb.h"
#include "base/logger/LogStream.h"

// std::string Trade::Dump() const
// {
//   // static std::map<Side, const char*> sides = {
//   //   {Side::Buy, "Buy"}, {Side::Sell, "Sell"}, {Side::BuyCover, "BuyCover"},
//   //   {Side::BuyCoverToday, "BuyCoverToday"}, {Side::BuyCoverYesterday, "BuyCoverYesterDay"},
//   //   {Side::SellCover, "SellCover"}, {Side::SellCoverToday, "SellCoverToday"},
//   //   {Side::SellCoverYesterday, "SellCoverYesterday"} };
//   return (boost::format("%1% %2% %3% %4% %5%@%6%") % id % instrument->Id() % order_id %
//       Proto::Side_Name(side) % volume % price).str();
// }

std::shared_ptr<Proto::Trade> Trade::Serialize() const {
  auto t = Message<Proto::Trade>::New();
  t->set_id(id);
  t->set_instrument(instrument->Id());
  t->set_exchange(instrument->Exchange());
  t->set_side(side);
  t->set_price(price);
  t->set_volume(volume);
  // google::protobuf::Timestamp *tm = new google::protobuf::Timestamp;
  // tm->set_seconds(header.time / 1000000);
  // tm->set_nanos(header.time % 1000000 * 1000);
  // t->set_allocated_time(tm);
  t->set_time(time);
  t->set_order_id(order_id);
  return t;
}

namespace base {

LogStream& operator<<(LogStream& stream, const TradePtr &trade) {
  assert(trade);
  stream << trade->id << ' ' << trade->instrument->Id() << ' ' << trade->order_id << ' '
         << Proto::Side_Name(trade->side) << ' ' << trade->volume << '@' << trade->price;
  return stream;
}

} // namespace base
