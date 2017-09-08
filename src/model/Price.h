#ifndef MODEL_PRICE_H
#define MODEL_PRICE_H

#include "Message.h"
#include "Instrument.h"
#include <memory>

struct Price
{
  Price() : header(MsgType::Price) {}

  MsgHeader header;
  const Instrument *instrument;
  base::PriceType last;
  base::PriceType bid;
  base::PriceType ask;
  base::PriceType open;
  base::PriceType high;
  base::PriceType low;
  base::PriceType close;
  double amount;
  // base::VolumeType last_volume;
  base::VolumeType bid_volume;
  base::VolumeType ask_volume;
  base::VolumeType volume;

  std::string DebugString();
};

typedef std::shared_ptr<Price> PricePtr;

#endif
