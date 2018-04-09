#ifndef MODEL_PRICE_H
#define MODEL_PRICE_H

#include "Message.h"
#include "Instrument.h"
#include <memory>

namespace Proto
{
  class Price;
}

struct PriceLevel
{
  operator bool() const { return volume != base::VOLUME_UNDEFINED; }

  base::PriceType price = base::PRICE_UNDEFINED;
  base::VolumeType volume = base::VOLUME_UNDEFINED;
};

struct Price
{
  static const int LEVELS = 10;
  Price() : header(MsgType::Price) {}

  MsgHeader header;
  const Instrument *instrument;
  PriceLevel last;
  PriceLevel bids[LEVELS];
  PriceLevel asks[LEVELS];
  base::PriceType open = base::PRICE_UNDEFINED;
  base::PriceType high = base::PRICE_UNDEFINED;
  base::PriceType low = base::PRICE_UNDEFINED;
  base::PriceType close = base::PRICE_UNDEFINED;
  base::PriceType pre_close = base::PRICE_UNDEFINED;
  base::PriceType pre_settlement = base::PRICE_UNDEFINED;
  base::PriceType amount = base::PRICE_UNDEFINED;
  base::VolumeType volume = base::VOLUME_UNDEFINED;

  std::string Dump() const;
  std::shared_ptr<Proto::Price> Serialize() const;
};

typedef std::shared_ptr<Price> PricePtr;

#endif
