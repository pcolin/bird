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
  base::PriceType last = base::PRICE_UNDEFINED;
  base::PriceType bid = base::PRICE_UNDEFINED;
  base::PriceType ask = base::PRICE_UNDEFINED;
  base::PriceType open = base::PRICE_UNDEFINED;
  base::PriceType high = base::PRICE_UNDEFINED;
  base::PriceType low = base::PRICE_UNDEFINED;
  base::PriceType close = base::PRICE_UNDEFINED;
  double amount = base::PRICE_UNDEFINED;
  // base::VolumeType last_volume;
  base::VolumeType bid_volume = base::VOLUME_UNDEFINED;
  base::VolumeType ask_volume = base::VOLUME_UNDEFINED;
  base::VolumeType volume = base::VOLUME_UNDEFINED;

  std::string Dump();
};

typedef std::shared_ptr<Price> PricePtr;

#endif
