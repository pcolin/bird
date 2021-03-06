#include "Middleware.h"
#include <sstream>
// #include "Message.h"
#include "ClientManager.h"
#include "InstrumentManager.h"
#include "ParameterManager.h"
#include "Instrument.pb.h"
#include "Credit.pb.h"
#include "exchange/manager/ExchangeManager.h"
#include "strategy/base/DeviceManager.h"
#include "strategy/base/ClusterManager.h"
#include "config/EnvConfig.h"
#include "nanomsg/pubsub.h"
#include "boost/format.hpp"

Middleware* Middleware::GetInstance() {
  static Middleware middleware;
  return &middleware;
}

Middleware::Middleware()
    : proto_messages_(kCapacity) {}

void Middleware::Init() {
  timer_ = std::make_unique<std::thread>(std::bind(&Middleware::RunTimer, this));
  publisher_ = std::make_unique<std::thread>(std::bind(&Middleware::RunPublisher, this));
  responder_ = std::make_unique<std::thread>(std::bind(&Middleware::RunResponder, this));
}

base::ProtoMessagePtr Middleware::Request(const google::protobuf::Message &request) {
  int req = nn_socket(AF_SP, NN_REQ);
  if (req < 0) {
    LOG_ERR << "failed to create req socket: " << nn_strerror(nn_errno());
    return nullptr;
  }
  const std::string addr = EnvConfig::GetInstance()->GetString(EnvVar::REQ_ADDR);
  if (nn_connect(req, addr.c_str()) < 0) {
    LOG_ERR << "failed to connect req socket: " << nn_strerror(nn_errno());
    nn_close(req);
    return nullptr;
  }

  base::ProtoMessagePtr reply = nullptr;
  size_t n;
  char *buffer = base::EncodeProtoMessage(request, n);
  if (buffer) {
    if (nn_send(req, buffer, n, 0) > 0) {
      LOG_INF << "success to send: " << request.ShortDebugString();
      void *msg = NULL;
      int rc = nn_recv(req, &msg, NN_MSG, 0);
      if (rc > 0) {
        LOG_DBG << boost::format("receive %1% bytes: %2%") % rc % base::GetString(msg, rc);
        reply = base::ProtoMessagePtr(base::DecodeProtoMessage(msg, rc));
        if (reply) {
          LOG_INF << "success to decode message: " << reply->ShortDebugString();
        } else {
          LOG_INF << "failed to decode message";
        }
        nn_freemsg(msg);
      } else {
        LOG_ERR << "failed to receive message: " << nn_strerror(nn_errno());
      }
    } else {
      LOG_ERR << "failed to send message: " << nn_strerror(nn_errno());
    }
    delete[] buffer;
  } else {
    LOG_ERR << "failed to encode message";
  }
  nn_close(req);
  return reply;
}

// Middleware::ProtoReplyPtr Middleware::OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &msg)
// {
//   if (msg->type() == Proto::ProcessorType::GUI)
//   {
//     ClientManager::GetInstance()->OnHeartbeat(msg);
//     auto reply = Message::NewProto<Proto::Reply>();
//     reply->set_result(true);
//     reply->set_error("");
//     return reply;
//   }
// }

std::shared_ptr<Proto::Reply> Middleware::OnOrderRequest(
    const std::shared_ptr<Proto::OrderRequest> &req) {
  LOG_INF << "OnOrderRequest: " << req->ShortDebugString();
  if (req->action() == Proto::OrderAction::Submit) {
    for (auto &ord : req->orders()) {
      auto *inst = InstrumentManager::GetInstance()->FindId(ord.instrument());
      if (inst) {
        auto order = std::make_shared<Order>();
        order->instrument = inst;
        // order->id = ord.id();
        order->header.time = ord.time();
        order->price = ord.price();
        order->volume = ord.volume();
        order->strategy_type = ord.strategy_type();
        order->side = ord.side();
        order->time_condition = ord.time_condition();
        order->type = ord.type();
        order->status = ord.status();
        ExchangeManager::GetInstance()->GetTraderApi()->Submit(order);
      }
    }
  } else if (req->action() == Proto::OrderAction::Cancel) {
    for (auto &ord : req->orders()) {
      auto *inst = InstrumentManager::GetInstance()->FindId(ord.instrument());
      if (inst) {
        auto order = std::make_shared<Order>();
        order->instrument = inst;
        order->id = ord.id();
        order->counter_id = ord.counter_id();
        order->exchange_id = ord.exchange_id();
        order->header.time = ord.time();
        order->price = ord.price();
        order->volume = ord.volume();
        order->executed_volume = ord.executed_volume();
        order->strategy_type = ord.strategy_type();
        order->side = ord.side();
        order->time_condition = ord.time_condition();
        order->type = ord.type();
        order->status = ord.status();
        ExchangeManager::GetInstance()->GetTraderApi()->Cancel(order);
      }
    }
  }
  return nullptr;
}

void Middleware::RunTimer() {
  LOG_INF << "start middleware timer...";
  auto heartbeat = std::make_shared<Proto::Heartbeat>();
  Proto::Exchange exchange;
  Proto::Exchange_Parse(EnvConfig::GetInstance()->GetString(EnvVar::EXCHANGE), &exchange);
  heartbeat->set_exchange(exchange);
  heartbeat->set_type(Proto::ProcessorType::Middleware);
  while (true) {
    ClusterManager::GetInstance()->OnHeartbeat(heartbeat);
    ClientManager::GetInstance()->OnHeartbeat(heartbeat);
    Publish(heartbeat);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

void Middleware::RunPublisher() {
  LOG_INF << "start middleware publishing thread...";
  int pub = nn_socket(AF_SP, NN_PUB);
  if (pub < 0) {
    LOG_ERR << "failed to create pub socket: " << nn_strerror(nn_errno());
    return;
  }
  const std::string pub_addr = EnvConfig::GetInstance()->GetString(EnvVar::BIND_SUB_ADDR);
  if (nn_bind(pub, pub_addr.c_str()) < 0) {
    LOG_ERR << "failed to bind pub socket(" << pub_addr << "):" << nn_strerror(nn_errno());
    nn_close(pub);
    return;
  }
  ProtoMessagePtr messages[kCapacity];
  size_t n = 1024;
  char *buf = new char[n];
  while (true) {
    size_t cnt = proto_messages_.wait_dequeue_bulk(messages, kCapacity);
    for (size_t i = 0; i < cnt; ++i) {
      size_t size = base::EncodeProtoMessage(*messages[i], &buf, n);
      if (size > 0) {
        size_t bytes  = nn_send(pub, buf, size, 0);
        if (unlikely(bytes != size)) {
          LOG_ERR << boost::format("failed to publish message(%1% != %2%)") % bytes % size;
        } else {
          // LOG_INF << boost::format("Success to publish %1%: %2%") % messages[i]->GetTypeName() %
          //   messages[i]->ShortDebugString();
          LOG_TRA << "publish " << messages[i]->GetTypeName() << ":"
                  << messages[i]->ShortDebugString();
        }
      } else {
        LOG_ERR << boost::format("failed to encode %1%: %2%") % messages[i]->GetTypeName() %
                   messages[i]->ShortDebugString();
      }
    }
  }
  nn_close(pub);
}

void Middleware::RunResponder() {
  LOG_INF << "start middleware responder thread...";
  int sock = nn_socket(AF_SP, NN_REP);
  if (sock < 0) {
    LOG_ERR << "failed to create responser socket: " << nn_strerror(nn_errno());
    return;
  }
  const std::string addr = EnvConfig::GetInstance()->GetString(EnvVar::BIND_REQ_ADDR);
  if (nn_bind(sock, addr.c_str()) < 0) {
    LOG_ERR << "failed to bind responser socket: " << nn_strerror(nn_errno());
    nn_close(sock);
    return;
  }

  base::ProtoMessageDispatcher<std::shared_ptr<Proto::Reply>> dispatcher;
  dispatcher.RegisterCallback<Proto::OrderRequest>(std::bind(
      &Middleware::OnOrderRequest, this, std::placeholders::_1));
  dispatcher.RegisterCallback<Proto::Heartbeat>(std::bind(
      &ClientManager::OnHeartbeat, ClientManager::GetInstance(), std::placeholders::_1));
  dispatcher.RegisterCallback<Proto::Login>(std::bind(
      &ClientManager::Login, ClientManager::GetInstance(), std::placeholders::_1));
  dispatcher.RegisterCallback<Proto::Logout>(std::bind(
      &ClientManager::Logout, ClientManager::GetInstance(), std::placeholders::_1));
  dispatcher.RegisterCallback<Proto::ProductParameterReq>(std::bind(
      &ParameterManager::OnProductParameterReq, ParameterManager::GetInstance(),
      std::placeholders::_1));
  dispatcher.RegisterCallback<Proto::InterestRateReq>(std::bind(
      &ParameterManager::OnInterestRateReq, ParameterManager::GetInstance(),
      std::placeholders::_1));
  dispatcher.RegisterCallback<Proto::SSRateReq>(std::bind(
      &ParameterManager::OnSSRateReq, ParameterManager::GetInstance(), std::placeholders::_1));
  dispatcher.RegisterCallback<Proto::VolatilityCurveReq>(std::bind(
      &ParameterManager::OnVolatilityCurveReq, ParameterManager::GetInstance(),
      std::placeholders::_1));
  dispatcher.RegisterCallback<Proto::DestrikerReq>(std::bind(
      &ParameterManager::OnDestrikerReq, ParameterManager::GetInstance(), std::placeholders::_1));
  dispatcher.RegisterCallback<Proto::PriceReq>(std::bind(
      &ClusterManager::OnPriceReq, ClusterManager::GetInstance(), std::placeholders::_1));
  dispatcher.RegisterCallback<Proto::PricerReq>(std::bind(
      &ClusterManager::OnPricerReq, ClusterManager::GetInstance(), std::placeholders::_1));
  dispatcher.RegisterCallback<Proto::CreditReq>(std::bind(
      &ClusterManager::OnCreditReq, ClusterManager::GetInstance(), std::placeholders::_1));
  dispatcher.RegisterCallback<Proto::QuoterReq>(std::bind(
      &ClusterManager::OnQuoterReq, ClusterManager::GetInstance(), std::placeholders::_1));
  dispatcher.RegisterCallback<Proto::StrategySwitchReq>(std::bind(
      &ClusterManager::OnStrategySwitchReq, ClusterManager::GetInstance(), std::placeholders::_1));
  dispatcher.RegisterCallback<Proto::StrategyOperateReq>(std::bind(
      &ClusterManager::OnStrategyOperateReq, ClusterManager::GetInstance(),std::placeholders::_1));
  size_t n = 64;
  char *send_buf = new char[n];
  auto reply = std::make_shared<Proto::Reply>();
  while (true) {
    char *recv_buf = NULL;
    int recv_bytes = nn_recv(sock, &recv_buf, NN_MSG, 0);
    if (recv_bytes > 0) {
      std::shared_ptr<google::protobuf::Message> msg(base::DecodeProtoMessage(recv_buf, recv_bytes));
      if (msg) {
        // LOG_INF << boost::format("type: %1% %2%") % msg->GetTypeName() % msg->ShortDebugString();
        LOG_INF << msg->GetTypeName() << " " << msg->ShortDebugString();
        auto r = dispatcher.OnProtoMessage(msg);
        if (r) {
          reply->set_result(r->result());
          reply->set_error(r->error());
        } else {
          reply->set_result(true);
          reply->set_error("");
        }
        Publish(msg);
      } else {
        reply->set_result(false);
        reply->set_error("decode message failed");
        LOG_ERR << "failed to decode proto message";
      }
      size_t size = base::EncodeProtoMessage(*reply, &send_buf, n);
      if (size > 0) {
        size_t bytes = nn_send(sock, send_buf, size, 0);
        if (unlikely(bytes != size)) {
          LOG_ERR << boost::format("failed to send message(%1% != %2%)") % bytes % size;
        }
      }
      nn_freemsg(recv_buf);
    } else { // if (nn_errno() != EINTR)
      // reply->set_result(false);
      // reply->set_error("Receive message failed");
      LOG_ERR << "failed to receive message: " << nn_strerror(nn_errno());
    }
    // LOG_INF << reply->ShortDebugString();
  }
  delete[] send_buf;
  nn_close(sock);
}
