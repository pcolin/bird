#ifndef MODEL_MIDDLEWARE_H
#define MODEL_MIDDLEWARE_H

#include "base/concurrency/blockingconcurrentqueue.h"
#include "Heartbeat.pb.h"
#include "Login.pb.h"
#include "Strategy.pb.h"
#include <thread>

class Middleware
{
  typedef std::shared_ptr<google::protobuf::Message> ProtoMessagePtr;
public:
  static Middleware* GetInstance();
  ~Middleware() {}

  void Init();

  template<class ProtoType> void Publish(const std::shared_ptr<ProtoType> &msg)
  {
    proto_messages_.enqueue(msg);
  }

  // template<class Type> void Publish(const std::shared_ptr<Type> &msg)
  // {
  //   ProtoMessagePtr pmp(Type::Serialize(msg));
  //   if (pmp)
  //     Publish(pmp);
  // }

private:
  Middleware();
  ProtoMessagePtr OnHeartbeat(const std::shared_ptr<proto::Heartbeat> &msg);
  ProtoMessagePtr OnLogin(const std::shared_ptr<proto::Login> &msg);
  ProtoMessagePtr OnLogout(const std::shared_ptr<proto::Logout> &msg);
  ProtoMessagePtr OnStrategyStatusReq(const std::shared_ptr<proto::StrategyStatusReq> &msg);

  void RunTimer();
  void RunPublisher();
  void RunResponder();

  std::unique_ptr<std::thread> timer_;
  std::unique_ptr<std::thread> publisher_;
  std::unique_ptr<std::thread> responder_;

  moodycamel::BlockingConcurrentQueue<ProtoMessagePtr> proto_messages_;
  static const size_t capacity_ = 1024;

};

#endif
