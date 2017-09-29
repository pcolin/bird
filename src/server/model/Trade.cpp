#include "Trade.h"
#include <map>
#include <boost/format.hpp>

std::string Trade::Dump() const
{
  static std::map<Side, const char*> sides = {
    {Side::Buy, "Buy"}, {Side::Sell, "Sell"}, {Side::BuyCover, "BuyCover"},
    {Side::BuyCoverToday, "BuyCoverToday"}, {Side::BuyCoverYesterday, "BuyCoverYesterDay"},
    {Side::SellCover, "SellCover"}, {Side::SellCoverToday, "SellCoverToday"},
    {Side::SellCoverYesterday, "SellCoverYesterday"} };
  return (boost::format("%1% %2% %3% %4% %5%@%6%") % id % instrument->Id() % order_id % sides[side] %
      volume % price).str();
}
