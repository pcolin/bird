#ifndef BASE_PROTO_MESSAGE_DISPATCHER_H
#define BASE_PROTO_MESSAGE_DISPATCHER_H

#include "google/protobuf/message.h"
#include <unordered_map>

namespace base
{

typedef std::shared_ptr<google::protobuf::Message> ProtoMessagePtr;
template<class R>
class Callback
{
public:
  virtual R OnMessage(const ProtoMessagePtr &msg) = 0;
};

template<class R, class T>
class CallbackT : public Callback<R>
{
public:
  typedef std::function<R(const std::shared_ptr<T>&)> MsgCallback;
  CallbackT(const MsgCallback &cb) : cb_(cb)
  {}

  virtual R OnMessage(const ProtoMessagePtr &msg) override
  {
    auto p = std::static_pointer_cast<T>(msg);
    return cb_(p);
  }

private:
  MsgCallback cb_;
};

template<class R>
class ProtoMessageDispatcher
{
public:
  R OnProtoMessage(const ProtoMessagePtr &msg) const
  {
    auto it = callbacks_.find(msg->GetDescriptor());
    if (it != callbacks_.end())
    {
      return it->second->OnMessage(msg);
    }
    return R();
  }

  template<class T>
  void RegisterCallback(const typename CallbackT<R, T>::MsgCallback &cb)
  {
    callbacks_.emplace(T::descriptor(), std::make_shared<CallbackT<R, T>>(cb));
  }

private:
  std::unordered_map<const google::protobuf::Descriptor*, std::shared_ptr<Callback<R>>> callbacks_;
};

// class ProtoMessageFunction
// {
//   typedef std::shared_ptr<google::protobuf::Message> ProtoMessagePtr;
//   class Function
//   {
//   public:
//     virtual ProtoMessagePtr OnMessage(const ProtoMessagePtr &msg) = 0;
//   };

//   template<class T>
//   class FunctionT : public Function
//   {
//   public:
//     typedef std::function<ProtoMessagePtr(const std::shared_ptr<T>&)> MsgFunction;
//     FunctionT(const MsgFunction &func) : func_(func)
//     {}

//     virtual ProtoMessagePtr OnMessage(const ProtoMessagePtr &msg) override
//     {
//       auto p = std::static_pointer_cast<T>(msg);
//       return func_(p);
//     }

//   private:
//     MsgFunction func_;
//   };

// public:
//   ProtoMessagePtr OnProtoMessage(const ProtoMessagePtr &msg) const
//   {
//     auto it = callbacks_.find(msg->GetDescriptor());
//     if (it != callbacks_.end())
//     {
//       return it->second->OnMessage(msg);
//     }
//     return nullptr;
//   }

//   template<class T>
//   void RegisterFunction(const typename FunctionT<T>::MsgFunction &cb)
//   {
//     callbacks_.emplace(T::descriptor(), std::make_shared<FunctionT<T>>(cb));
//   }

// private:
//   std::unordered_map<const google::protobuf::Descriptor*, std::shared_ptr<Function>> callbacks_;
// };

}
#endif
