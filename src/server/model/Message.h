#ifndef MODEL_MESSAGE_H
#define MODEL_MESSAGE_H

#include "base/common/Types.h"
#include <memory>

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

class Price;
class Order;
class Trade;
class Message
{
public:
  static std::shared_ptr<Price> NewPrice();
  static std::shared_ptr<Order> NewOrder();
  static std::shared_ptr<Order> NewOrder(const std::shared_ptr<Order> &ord);
  static std::shared_ptr<Trade> NewTrade();
};

#endif
