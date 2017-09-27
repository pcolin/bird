#ifndef STRATEGY_PROTO_MESSAGE_DISPATCHER_H
#define STRATEGY_PROTO_MESSAGE_DISPATCHER_H

#include "StrategyTypes.h"
#include <unordered_map>

class Callback
{
public:
  virtual void OnMessage(const ProtoMessagePtr &msg) = 0;
};

template<class T>
class CallbackT : public Callback
{
public:
  typedef std::function<void(const std::shared_ptr<T>&)> MsgCallback;
  CallbackT(const MsgCallback &cb) : cb_(cb)
  {}

  virtual void OnMessage(const ProtoMessagePtr &msg) override
  {
    auto p = std::static_pointer_cast<T>(msg);
    cb_(p);
  }

private:
  MsgCallback cb_;
};

class ProtoMessageDispatcher
{
public:
  void OnProtoMessage(const ProtoMessagePtr &msg) const
  {
    auto it = callbacks_.find(msg->GetDescriptor());
    if (it != callbacks_.end())
    {
      it->second->OnMessage(msg);
    }
  }

  template<class T>
  void RegisterCallback(const typename CallbackT<T>::MsgCallback &cb)
  {
    callbacks_.emplace(T::descriptor(), std::make_shared<CallbackT<T>>(cb));
  }

private:
  std::unordered_map<const google::protobuf::Descriptor*, std::shared_ptr<Callback>> callbacks_;
};

#endif
