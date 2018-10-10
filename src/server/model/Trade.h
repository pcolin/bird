#ifndef MODEL_TRADE_H
#define MODEL_TRADE_H

#include <memory>
#include "Order.h"
#include "Trade.pb.h"

struct Trade {
  Trade() : header(MsgType::Trade) {}
  // std::string Dump() const;
  std::shared_ptr<Proto::Trade> Serialize() const;

  MsgHeader header;
  const Instrument *instrument;
  std::string id;
  Proto::Side side;
  base::PriceType price;
  base::VolumeType volume;
  int64_t time;
  size_t order_id;
};

typedef std::shared_ptr<Trade> TradePtr;

namespace base {

class LogStream;
LogStream& operator<<(LogStream& stream, const TradePtr &order);

} // namespace base

#endif // MODEL_TRADE_H
