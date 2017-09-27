#include "Trade.h"
#include <boost/format.hpp>

std::string Trade::Dump() const
{
  return (boost::format("%1% %2% %3% %4% %5%@%6%") % id % instrument->Id() % order_id %
      (side == Side::Buy ? "Buy" : "Sell") % volume % price).str();
}
