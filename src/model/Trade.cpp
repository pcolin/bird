#include "Trade.h"
#include <boost/format.hpp>

std::string Trade::Dump() const
{
  return (boost::format("%1% %2%") % id % instrument->Id()).str();
}
