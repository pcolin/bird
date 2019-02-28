#ifndef MODEL_MESSAGE_H
#define MODEL_MESSAGE_H

#include <mutex>
#include <memory>
#include <cassert>
#include "base/common/Types.h"
#include "base/common/Time.h"
#include "base/memory/MemoryPool.h"

enum class MsgType : int8_t {
  Price = 0,
  TheoMatrix = 1,
  Order = 2,
  Trade = 3,
};

struct MsgHeader {
  MsgType type;
  int32_t interval[3];
  int64_t time;

  MsgHeader(MsgType t) : type(t) {}
  void SetTime() { time = base::Now(); }
  void SetInterval(int idx) {
    assert(idx < 3);
    interval[idx] = static_cast<int32_t>(base::Now() - time);
  }
};

template<class T>
class MessageFactory {
 public:
  // ~MessageFactory()
  // {
  //   std::cout << "~MessageFactory" << std::endl;
  // }
  std::shared_ptr<T> Allocate() {
    T *t = nullptr;
    {
      std::lock_guard<std::mutex> lck(mtx_);
      t = pool_.newElement();
    }
    assert(t);
    return std::shared_ptr<T>(t, [this](T *p) {
          std::lock_guard<std::mutex> lck(mtx_);
          pool_.deleteElement(p);
        });
  }

  std::shared_ptr<T> Allocate(const T &t) {
    T *p = nullptr;
    {
      std::lock_guard<std::mutex> lck(mtx_);
      p = pool_.newElement(t);
    }
    assert(p);
    return std::shared_ptr<T>(p, [this](T *pt) {
          std::lock_guard<std::mutex> lck(mtx_);
          pool_.deleteElement(pt);
        });
  }

  // std::shared_ptr<T> Allocate(const std::shared_ptr<T> &pt) {
  //   T *t = nullptr;
  //   {
  //     std::lock_guard<std::mutex> lck(mtx_);
  //     t = pool_.newElement(*pt);
  //   }
  //   assert(t);
  //   return std::shared_ptr<T>(t, [this](T *p) {
  //         std::lock_guard<std::mutex> lck(mtx_);
  //         pool_.deleteElement(p);
  //       });
  // }

 private:
  MemoryPool<T, 1024*sizeof(T)> pool_;
  /// FIXME: spin lock?
  std::mutex mtx_;
};

template<class T>
class Message {
 public:
  static std::shared_ptr<T> New() { return factory.Allocate(); }
  static std::shared_ptr<T> New(const T &t) { return factory.Allocate(t); }
  static std::shared_ptr<T> New(const std::shared_ptr<T> &t) {return factory.Allocate(*t);}

 private:
  static MessageFactory<T> factory;
};

template<class T> MessageFactory<T> Message<T>::factory;

#endif // MODEL_MESSAGE_H
