#include "Price.h"
#include <boost/format.hpp>

std::string Price::DebugString()
{
  return (boost::format("instrument(%1%), last(%2%), bid(%3%), ask(%4%), open(%5%), high(%6%), "
    "low(%7%), close(%8%), amount(%9%), bid_volume(%10%), ask_volume(%11%), volume(%12%)")
    % instrument->Id() % last % bid % ask % open % high % low % close % amount % bid_volume
    % ask_volume % volume).str();
}
