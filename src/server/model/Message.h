#ifndef MODEL_MESSAGE_H
#define MODEL_MESSAGE_H

#include "base/common/Types.h"
#include "base/common/Time.h"
#include "base/memory/MemoryPool.h"
#include <mutex>
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
  void SetTime() { time = base::Now(); }
  void SetInterval(int idx) { interval[idx] = static_cast<int32_t>(base::Now() - time); }
};

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
  MemoryPool<T, 1024*sizeof(T)> pool_;
  std::mutex mtx_;
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
  template<class T> static std::shared_ptr<T> NewProto()
  {
    static MessageFactory<T> factory;
    return factory.Allocate();
  }

  // static int64_t GetTime();
};

#endif
