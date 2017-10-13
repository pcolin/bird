#include "Price.h"
#include "Order.h"
#include "Trade.h"

// #include "base/memory/MemoryPool.h"

// #include <iostream>
#include <mutex>
#include <sys/time.h>

void MsgHeader::SetTime()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  time = tv.tv_sec;
  time = time * 1000000 + tv.tv_usec;
}

void MsgHeader::SetInterval(int idx)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int64_t now = tv.tv_sec;
  now = now * 1000000 + tv.tv_usec;
  interval[idx] = static_cast<int32_t>(now - time);
}

PricePtr Message::NewPrice()
{
  static MessageFactory<Price> factory;
  return factory.Allocate();
}

static MessageFactory<Order> order_factory;
OrderPtr Message::NewOrder()
{
  return order_factory.Allocate();
}

OrderPtr Message::NewOrder(const OrderPtr &ord)
{
  return order_factory.Allocate(ord);
}

TradePtr Message::NewTrade()
{
  static MessageFactory<Trade> factory;
  return factory.Allocate();
}
