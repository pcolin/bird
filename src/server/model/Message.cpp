#include "Price.h"
#include "TheoMatrix.h"
#include "Order.h"
#include "Trade.h"

#include <mutex>

PricePtr Message::NewPrice()
{
  static MessageFactory<Price> factory;
  return factory.Allocate();
}

TheoMatrixPtr Message::NewTheoMatrix()
{
  static MessageFactory<TheoMatrix> factory;
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
