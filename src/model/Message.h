#ifndef MODEL_MESSAGE_H
#define MODEL_MESSAGE_H

#include "base/common/Types.h"
#include "Instrument.h"

enum class MsgType : int8_t
{
  Price = 0,
  Order = 1,
  Trade = 2,
};

struct MsgHeader
{
  MsgType type;
  int32_t interval[3];
  int64_t time;

  MsgHeader(MsgType t) : type(t) {}
  void SetTime();
  void SetInterval(int idx);
};

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
};

struct Order
{
  Order() : header(MsgType::Order) {}

  MsgHeader header;
  const Instrument *instrument;
};

struct Trade
{
  Trade() : header(MsgType::Trade) {}

  MsgHeader header;
  const Instrument *instrument;
};

#endif
