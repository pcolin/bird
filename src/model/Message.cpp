#include "Price.h"
#include "Order.h"
#include "Trade.h"

#include "base/memory/MemoryPool.h"

#include <iostream>
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

template<class T>
class MessageFactory
{
public:
  // ~MessageFactory()
  // {
  //   std::cout << "~MessageFactory" << std::endl;
  // }

  std::shared_ptr<T> Allocate()
  {
    T *t = nullptr;
    {
      std::lock_guard<std::mutex> lck(mtx_);
      t = pool_.newElement();
    }
    assert(t);
    return std::shared_ptr<T>(t, [this](T *p)
        {
          std::lock_guard<std::mutex> lck(mtx_);
          pool_.deleteElement(p);
        });
  }

  std::shared_ptr<T> Allocate(const std::shared_ptr<T> &pt)
  {
    T *t = nullptr;
    {
      std::lock_guard<std::mutex> lck(mtx_);
      t = pool_.newElement(*pt);
    }
    assert(t);
    return std::shared_ptr<T>(t, [this](T *p)
        {
          std::lock_guard<std::mutex> lck(mtx_);
          pool_.deleteElement(p);
        });
  }
private:
  MemoryPool<T, 1000*sizeof(T)> pool_;
  std::mutex mtx_;
};

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
