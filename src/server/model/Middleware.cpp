#include "Middleware.h"
#include "Message.h"
#include "ProtoMessageDispatcher.h"
#include "ProductManager.h"
#include "Strategy.pb.h"
#include "base/logger/Logging.h"
#include "base/common/ProtoMessageCoder.h"
#include "config/EnvConfig.h"
#include "strategy/DeviceManager.h"
#include "strategy/ClusterManager.h"

#include "nn.h"
#include "pubsub.h"
#include "reqrep.h"

#include <boost/format.hpp>

Middleware* Middleware::GetInstance()
{
  static Middleware middleware;
  return &middleware;
}

Middleware::Middleware()
  : proto_messages_(capacity_)
{}

void Middleware::Init()
{
  timer_ = std::make_unique<std::thread>(std::bind(&Middleware::RunTimer, this));
  publisher_ = std::make_unique<std::thread>(std::bind(&Middleware::RunPublisher, this));
  responder_ = std::make_unique<std::thread>(std::bind(&Middleware::RunResponder, this));
}

ProtoMessagePtr Middleware::OnHeartbeat(const std::shared_ptr<proto::Heartbeat> &msg)
{}

ProtoMessagePtr Middleware::OnLogin(const std::shared_ptr<proto::Login> &msg)
{
  LOG_PUB << boost::format("%1% login") % msg->user();
  return Message::NewProto<proto::LoginRep>();
}

ProtoMessagePtr Middleware::OnLogout(const std::shared_ptr<proto::Logout> &msg)
{
  LOG_PUB << boost::format("%1% logout") % msg->user();
  return Message::NewProto<proto::LogoutRep>();
}

ProtoMessagePtr Middleware::OnStrategyStatusReq(const std::shared_ptr<proto::StrategyStatusReq> &msg)
{
  if (msg->type() == proto::RequestType::Set)
  {
    for (auto &status : msg->statuses())
    {
      const Instrument *underlying = ProductManager::GetInstance()->FindId(status.underlying());
      if (underlying)
      {
        auto *dm = ClusterManager::GetInstance()->FindDevice(underlying);
        if (dm)
        {
          dm->OnStrategyStatusReq(msg); 
        }
      }
      else
      {
        LOG_ERR << "Can't find underlying " << status.underlying();
      }
    }
  }
  return Message::NewProto<proto::StrategyStatusRep>();
}

void Middleware::RunTimer()
{
  LOG_INF << "Start middleware timer...";
  auto heartbeat = Message::NewProto<proto::Heartbeat>();
  while (true)
  {
    ClusterManager::GetInstance()->OnHeartbeat(heartbeat);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

void Middleware::RunPublisher()
{
  LOG_INF << "Start middleware publishing thread...";
  int pub = nn_socket(AF_SP, NN_PUB);
  if (pub < 0)
  {
    LOG_ERR << "Failed to create pub socket: " << nn_strerror(nn_errno());
    return;
  }
  const std::string pub_addr = EnvConfig::GetInstance()->GetString(EnvVar::BIND_SUB_ADDR);
  if (nn_bind(pub, pub_addr.c_str()) < 0)
  {
    LOG_ERR << "Failed to bind pub socket: " << nn_strerror(nn_errno());
    nn_close(pub);
    return;
  }
  ProtoMessagePtr messages[capacity_];
  size_t n = 1024;
  char *buf = new char[n];
  while (true)
  {
    size_t cnt = proto_messages_.wait_dequeue_bulk(messages, capacity_);
    for (size_t i = 0; i < cnt; ++i)
    {
      size_t size = base::EncodeProtoMessage(*messages[i], &buf, n);
      if (size > 0)
      {
        size_t bytes  = nn_send(pub, buf, size, 0);
        if (unlikely(bytes != size))
        {
          LOG_ERR << boost::format("Failed to publish message(%1% != %2%)") % bytes % size;
        }
        else
        {
          LOG_INF << "Success to publish message " << messages[i]->ShortDebugString();
        }
      }
    }
  }
  nn_close(pub);
}

void Middleware::RunResponder()
{
  LOG_INF << "Start middleware responder thread...";
  int sock = nn_socket(AF_SP, NN_REP);
  if (sock < 0)
  {
    LOG_ERR << "Failed to create responser socket: " << nn_strerror(nn_errno());
    return;
  }
  const std::string addr = EnvConfig::GetInstance()->GetString(EnvVar::BIND_REQ_ADDR);
  if (nn_bind(sock, addr.c_str()) < 0)
  {
    LOG_ERR << "Failed to bind responser socket: " << nn_strerror(nn_errno());
    nn_close(sock);
    return;
  }
  ProtoMessageDispatcher<ProtoMessagePtr> dispatcher;
  dispatcher.RegisterCallback<proto::Heartbeat>(
    std::bind(&Middleware::OnHeartbeat, this, std::placeholders::_1));
  dispatcher.RegisterCallback<proto::Login>(
    std::bind(&Middleware::OnLogin, this, std::placeholders::_1));
  dispatcher.RegisterCallback<proto::Logout>(
    std::bind(&Middleware::OnLogout, this, std::placeholders::_1));
  dispatcher.RegisterCallback<proto::StrategyStatusReq>(
    std::bind(&Middleware::OnStrategyStatusReq, this, std::placeholders::_1));
  size_t n = 64;
  char *send_buf = new char[n];
  while (true)
  {
    char *recv_buf = NULL;
    int recv_bytes = nn_recv(sock, &recv_buf, NN_MSG, 0);
    if (unlikely(recv_bytes < 0))
    {
      LOG_ERR << "Failed to receive message: " << nn_strerror(nn_errno());
      continue;
    }
    std::shared_ptr<google::protobuf::Message> msg(base::DecodeProtoMessage(recv_buf, recv_bytes));
    if (msg)
    {
      LOG_INF << boost::format("type: %1% %2%") % msg->GetTypeName() % msg->ShortDebugString();
      auto reply = dispatcher.OnProtoMessage(msg);
      if (reply)
      {
        size_t size = base::EncodeProtoMessage(*reply, &send_buf, n);
        if (size > 0)
        {
          size_t bytes = nn_send(sock, send_buf, size, 0);
          if (unlikely(bytes != size))
          {
            LOG_ERR << boost::format("Failed to send message(%1% != %2%)") % bytes % size;
          }
        }
      }
    }
    else
    {
      LOG_ERR << "Failed to decode proto message";
    }
    nn_freemsg(recv_buf);
  }
  delete[] send_buf;
  nn_close(sock);
}
