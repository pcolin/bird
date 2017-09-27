#ifndef MODEL_TRADE_H
#define MODEL_TRADE_H

#include "Order.h"
#include <memory>

struct Trade
{
  Trade() : header(MsgType::Trade) {}
  std::string Dump() const;

  MsgHeader header;
  const Instrument *instrument;
  std::string id;
  Side side;
  base::PriceType price;
  base::VolumeType volume;
  int64_t time;
  size_t order_id;
};

typedef std::shared_ptr<Trade> TradePtr;

#endif
