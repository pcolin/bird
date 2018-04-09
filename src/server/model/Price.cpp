#include "Price.h"
#include "Price.pb.h"
// #include <boost/format.hpp>

std::string Price::Dump() const
{
  // return (boost::format("instrument(%1%), last(%2%), bid(%3%), ask(%4%), open(%5%), high(%6%), "
  //   "low(%7%), close(%8%), amount(%9%), bid_volume(%10%), ask_volume(%11%), volume(%12%)")
  //   % instrument->Id() % last % bid % ask % open % high % low % close % amount % bid_volume
  //   % ask_volume % volume).str();
  std::ostringstream oss;
  if (instrument)
  {
    oss << instrument->Id() << ':';
  }
  if (last.price != base::PRICE_UNDEFINED)
  {
    oss << " last(" << last.price << '/' << last.volume << ") ";
  }
  if (bids[0])
  {
    oss << " bids { ";
    for (int i = 0; i < LEVELS && bids[i]; ++i)
    {
      oss << bids[i].price << '/' << bids[i].volume << ' ';
    }
    oss << '}';
  }
  if (asks[0])
  {
    oss << " asks { ";
    for (int i = 0; i < LEVELS && asks[i]; ++i)
    {
      oss << asks[i].price << '/' << asks[i].volume << ' ';
    }
    oss << '}';
  }
  if (open != base::PRICE_UNDEFINED)
  {
    oss << " open(" << open << ") ";
  }
  if (high != base::PRICE_UNDEFINED)
  {
    oss << " high(" << high << ") ";
  }
  if (low != base::PRICE_UNDEFINED)
  {
    oss << " low(" << low << ") ";
  }
  if (close != base::PRICE_UNDEFINED)
  {
    oss << " close(" << close << ") ";
  }
  if (pre_close != base::PRICE_UNDEFINED)
  {
    oss << " pre_close(" << pre_close << ") ";
  }
  if (pre_settlement != base::PRICE_UNDEFINED)
  {
    oss << " pre_settlement(" << pre_settlement << ") ";
  }
  if (amount != base::PRICE_UNDEFINED)
  {
    oss << " amount(" << amount << ") ";
  }
  if (volume != base::VOLUME_UNDEFINED)
  {
    oss << " volume(" << volume << ") ";
  }
  return oss.str();
}

std::shared_ptr<Proto::Price> Price::Serialize() const
{
  auto p = Message::NewProto<Proto::Price>();
  p->set_instrument(instrument->Id());
  if (last.price != base::PRICE_UNDEFINED)
  {
    auto *l = p->mutable_last();
    l->set_price(last.price);
    l->set_volume(last.volume);
  }
  for(int i = 0; i < LEVELS && bids[i]; ++i)
  {
    auto *b = p->add_bids();
    b->set_price(bids[i].price);
    b->set_volume(bids[i].volume);
  }
  for(int i = 0; i < LEVELS && asks[i]; ++i)
  {
    auto *a = p->add_asks();
    a->set_price(asks[i].price);
    a->set_volume(asks[i].volume);
  }
  if (open != base::PRICE_UNDEFINED)
  {
    p->set_open(open);
  }
  if (high != base::PRICE_UNDEFINED)
  {
    p->set_high(high);
  }
  if (low != base::PRICE_UNDEFINED)
  {
    p->set_low(low);
  }
  if (close != base::PRICE_UNDEFINED)
  {
    p->set_close(close);
  }
  if (pre_close != base::PRICE_UNDEFINED)
  {
    p->set_pre_close(pre_close);
  }
  if (pre_settlement != base::PRICE_UNDEFINED)
  {
    p->set_pre_settlement(pre_settlement);
  }
  if (amount != base::PRICE_UNDEFINED)
  {
    p->set_amount(amount);
  }
  if (volume != base::VOLUME_UNDEFINED)
  {
    p->set_volume(volume);
  }
  return p;
}
