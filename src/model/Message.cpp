#include "Price.h"
#include "Order.h"
#include "Trade.h"

#include "base/memory/MemoryPool.h"

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
  std::shared_ptr<T> Allocate()
  {
    T *t = nullptr;
    {
      std::lock_guard<std::mutex> lck(mtx_);
      t = pool_.allocate();
    }
    assert(t);
    return std::shared_ptr<T>(t, [this](T *p)
        {
          std::lock_guard<std::mutex> lck(mtx_);
          pool_.deallocate(p);
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

OrderPtr Message::NewOrder()
{
  static MessageFactory<Order> factory;
  return factory.Allocate();
}

TradePtr Message::NewTrade()
{
  static MessageFactory<Trade> factory;
  return factory.Allocate();
}
