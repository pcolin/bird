#include "Price.h"
#include "Price.pb.h"
#include <boost/format.hpp>

std::string Price::Dump() const
{
  return (boost::format("instrument(%1%), last(%2%), bid(%3%), ask(%4%), open(%5%), high(%6%), "
    "low(%7%), close(%8%), amount(%9%), bid_volume(%10%), ask_volume(%11%), volume(%12%)")
    % instrument->Id() % last % bid % ask % open % high % low % close % amount % bid_volume
    % ask_volume % volume).str();
}

std::shared_ptr<proto::Price> Price::Serialize() const
{
  auto p = Message::NewProto<proto::Price>();
  p->set_instrument(instrument->Id());
  p->set_last(last);
  p->set_bid(bid);
  p->set_ask(ask);
  p->set_open(open);
  p->set_high(high);
  p->set_low(low);
  p->set_close(close);
  p->set_amount(amount);
  p->set_bid_volume(bid_volume);
  p->set_ask_volume(ask_volume);
  p->set_volume(volume);
  return p;
}
