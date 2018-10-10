#ifndef EXCHANGE_WASH_TRADE_PROTECTOR_H
#define EXCHANGE_WASH_TRADE_PROTECTOR_H

#include <list>
#include <unordered_map>
#include <mutex>
#include "model/Order.h"

class WashTradeProtector {
 public:
  WashTradeProtector();
  ~WashTradeProtector() {}

  bool TryAdd(const OrderPtr& order);
  bool TryAdd(const OrderPtr& bid, const OrderPtr& ask);
  void Remove(const OrderPtr& order);
  void Remove(const OrderPtr& bid, const OrderPtr& ask);

 private:
  const bool enabled_;
  typedef std::unordered_map<const Instrument*,
                             std::tuple<std::list<OrderPtr>, std::list<OrderPtr>>> InstOrderMap;
  InstOrderMap orders_;
  std::mutex mtx_;
};

#endif // EXCHANGE_WASH_TRADE_PROTECTOR_H
