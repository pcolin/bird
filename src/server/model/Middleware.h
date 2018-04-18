#ifndef MODEL_MIDDLEWARE_H
#define MODEL_MIDDLEWARE_H

#include "base/concurrency/blockingconcurrentqueue.h"
#include "base/logger/Logging.h"
#include "base/common/ProtoMessageCoder.h"
#include "base/common/ProtoMessageDispatcher.h"
#include "config/EnvConfig.h"
#include "Heartbeat.pb.h"
#include "Login.pb.h"
#include "Price.pb.h"
#include "Reply.pb.h"
#include "PricingSpec.pb.h"
#include "Strategy.pb.h"

#include "nn.h"
#include "reqrep.h"

#include <thread>

class Middleware
{
  // typedef std::shared_ptr<google::protobuf::Message> ProtoMessagePtr;
  typedef std::shared_ptr<Proto::Reply> ProtoReplyPtr;
public:
  static Middleware* GetInstance();
  ~Middleware() {}

  void Init();

  template<class ProtoType> void Publish(const std::shared_ptr<ProtoType> &msg)
  {
    proto_messages_.enqueue(msg);
  }

  base::ProtoMessagePtr Request(const base::ProtoMessagePtr &req);

  // template<class RequestType, class ReplyType>
  // std::shared_ptr<ReplyType> Request(const std::shared_ptr<RequestType> &request)
  // {
  //   int req = nn_socket(AF_SP, NN_REQ);
  //   if (req < 0)
  //   {
  //     LOG_ERR << "Failed to create req socket: " << nn_strerror(nn_errno());
  //     return nullptr;
  //   }
  //   const std::string addr = EnvConfig::GetInstance()->GetString(EnvVar::REQ_ADDR);
  //   if (nn_connect(req, addr.c_str()) < 0)
  //   {
  //     LOG_ERR << "Failed to connect req socket: " << nn_strerror(nn_errno());
  //     nn_close(req);
  //     return nullptr;
  //   }

  //   size_t n;
  //   char *buffer = base::EncodeProtoMessage(*request, n);
  //   if (buffer)
  //   {
  //     if (nn_send(req, buffer, n, 0) < 0)
  //     {
  //       LOG_ERR << "Failed to send message: " << nn_strerror(nn_errno());
  //       delete[] buffer;
  //       return nullptr;
  //     }
  //     delete[] buffer;

  //     void *msg = NULL;
  //     int rc = nn_recv(req, &msg, NN_MSG, 0);
  //     if (rc > 0)
  //     {
  //       auto *reply = base::DecodeProtoMessage(msg, rc);
  //       if (reply)
  //       {
  //         return std::shared_ptr<ReplyType>(dynamic_cast<ReplyType>(reply));
  //       }
  //       nn_freemsg(msg);
  //     }
  //     else
  //     {
  //     }
  //   }
  //   else
  //   {
  //     LOG_ERR << "Failed to encode message";
  //     nn_close(req);
  //     return nullptr;
  //   }
  // }

  // template<class Type> void Publish(const std::shared_ptr<Type> &msg)
  // {
  //   ProtoMessagePtr pmp(Type::Serialize(msg));
  //   if (pmp)
  //     Publish(pmp);
  // }

private:
  Middleware();
  ProtoReplyPtr OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &msg);
  ProtoReplyPtr OnLogin(const std::shared_ptr<Proto::Login> &msg);
  ProtoReplyPtr OnLogout(const std::shared_ptr<Proto::Logout> &msg);
  ProtoReplyPtr OnPriceReq(const std::shared_ptr<Proto::PriceReq> &msg);
  ProtoReplyPtr OnPricingSpecReq(const std::shared_ptr<Proto::PricingSpecReq> &msg);
  ProtoReplyPtr OnStrategyStatusReq(const std::shared_ptr<Proto::StrategyStatusReq> &msg);

  void RunTimer();
  void RunPublisher();
  void RunResponder();

  std::unique_ptr<std::thread> timer_;
  std::unique_ptr<std::thread> publisher_;
  std::unique_ptr<std::thread> responder_;

  moodycamel::BlockingConcurrentQueue<base::ProtoMessagePtr> proto_messages_;
  static const size_t capacity_ = 1024;

};

#endif
