#include "base/common/Version.h"
#include "base/common/ProtoMessageCoder.h"
#include "base/common/ProtoMessageDispatcher.h"
#include "base/concurrency/blockingconcurrentqueue.h"
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"
#include "UserDB.h"
#include "ExchangeParameterDB.h"
#include "InterestRateDB.h"
#include "SSRateDB.h"
#include "VolatilityCurveDB.h"
#include "CreditDB.h"
#include "DestrikerDB.h"
#include "CashLimitDB.h"
#include "PositionDB.h"
#include "OrderDB.h"
#include "TradeDB.h"
#include "PricerDB.h"
#include "StrategySwitchDB.h"
#include "QuoterDB.h"
#include "Heartbeat.pb.h"
#include "Server.pb.h"
#include "Price.pb.h"
#include "Order.pb.h"
#include "Trade.pb.h"
#include "Cash.pb.h"

#include "nn.h"
#include "pubsub.h"
#include "reqrep.h"

#include <tuple>
#include <thread>
#include <boost/format.hpp>

using namespace base;
using namespace moodycamel;

int InitRepSocket()
{
  int rep = nn_socket(AF_SP_RAW, NN_REP);
  if (rep < 0)
  {
    LOG_ERR << "Failed to create rep socket: " << nn_strerror(nn_errno());
    return -1;
  }

  const std::string rep_addr = EnvConfig::GetInstance()->GetString(EnvVar::BIND_REQ_ADDR);
  if (nn_bind(rep, rep_addr.c_str()) < 0)
  {
    LOG_ERR << "Failed to bind rep socket: " << nn_strerror(nn_errno());
    nn_close(rep);
    return -1;
  }
  return rep;
}

int InitSubSocket()
{
  int sub = nn_socket(AF_SP, NN_SUB);
  if (sub < 0)
  {
    LOG_ERR << "Failed to create sub socket: " << nn_strerror(nn_errno());
    return -1;
  }
  const std::string sub_addr = EnvConfig::GetInstance()->GetString(EnvVar::SUB_ADDR);
  if (nn_connect(sub, sub_addr.c_str()) < 0)
  {
    LOG_ERR << "Failed to connect sub socket: " << nn_strerror(nn_errno());
    nn_close(sub);
    return -1;
  }
  if (nn_setsockopt(sub, NN_SUB, NN_SUB_SUBSCRIBE, "", 0) < 0)
  {
    LOG_ERR << "Failed to set sub socket: " << nn_strerror(nn_errno());
    nn_close(sub);
    return -1;
  }
  return sub;
}

void ReplyRun(int rep, ProtoMessageDispatcher<ProtoMessagePtr> &dispatcher,
              BlockingConcurrentQueue<std::tuple<google::protobuf::Message*, void*>> &requests)
{
  LOG_INF << "DatabaseD reply thread begin to work...";
  const int32_t capacity = 128;
  std::tuple<google::protobuf::Message*, void*> messages[capacity];
  while (true)
  {
    size_t cnt = requests.wait_dequeue_bulk(messages, capacity);
    LOG_INF << "Get " << cnt << " messages";
    for (size_t i = 0; i < cnt; ++i)
    {
      void *buf = NULL;
      nn_msghdr hdr;
      memset(&hdr, 0, sizeof(hdr));
      hdr.msg_iov = NULL;
      hdr.msg_iovlen = 0;
      nn_iovec iov;
      auto *msg = std::get<0>(messages[i]);
      if (msg)
      {
        ProtoMessagePtr reply = dispatcher.OnProtoMessage(ProtoMessagePtr(msg));
        if (reply)
        {
          buf = nn_allocmsg(GetEncodedSize(*reply), 0);
          EncodeProtoMessage(*reply, static_cast<char*>(buf));
          iov.iov_base = &buf;
          iov.iov_len = NN_MSG;
          hdr.msg_iov = &iov;
          hdr.msg_iovlen = 1;
        }
        else
        {
          LOG_ERR << "Get null reply: " << msg->ShortDebugString();
        }
      }
      void *control = std::get<1>(messages[i]);
      hdr.msg_control = &control;
      hdr.msg_controllen = NN_MSG;
      if (nn_sendmsg(rep, &hdr, 0) < 0)
      {
        if (buf) nn_freemsg(buf);
        nn_freemsg(control);
        LOG_ERR << "Failed to send reply message: " << nn_strerror(nn_errno());
      }
    }
  }
}

void SubscribeRun(int sub, ProtoMessageDispatcher<ProtoMessagePtr> &dispatcher,
              BlockingConcurrentQueue<ProtoMessagePtr> &subscribes)
{
  LOG_INF << "DatabaseD subscribe thread begin to work...";
  const int32_t capacity = 128;
  ProtoMessagePtr messages[capacity];
  while (true)
  {
    size_t cnt = subscribes.wait_dequeue_bulk(messages, capacity);
    for (size_t i = 0; i < cnt; ++i)
    {
      dispatcher.OnProtoMessage(messages[i]);
    }
  }
}

int main(int argc, char *argv[])
{
  // char tmp[22] = {0};
  // size_t n = 15345678901234567890ULL;
  // sprintf(tmp, "%llu", n);
  // printf("%s\n", tmp);
  // return 0;

  Logger::InitFileLogger(EnvConfig::GetInstance()->GetString(EnvVar::LOGGING_DIR).c_str(),
                         EnvConfig::GetInstance()->GetString(EnvVar::APP_NAME).c_str(),
                         EnvConfig::GetInstance()->GetBool(EnvVar::ASYNC_LOGGING) == false);
  Logger::SetLogLevel(static_cast<Logger::LogLevel>(
        EnvConfig::GetInstance()->GetInt32(EnvVar::LOGGING_LEVEL)));
  LOG_INF << boost::format("Welcome BIRD-Database %1%.%2%.%3%") % VER_MAJOR % VER_MINOR % VER_PATCH;

  int rep = InitRepSocket();
  if (rep < 0) return -1;
  int sub = InitSubSocket();
  if (sub < 0) return -1;

  std::string db_file;
  if (EnvConfig::GetInstance()->GetValue(EnvVar::CONFIG_DB_FILE, db_file) == false)
  {
    LOG_FAT << "Failed to read CONFIG_DB_FILE";
    return -1;
  }
  ConcurrentSqliteDB config_db(db_file);

  if (EnvConfig::GetInstance()->GetValue(EnvVar::ORDER_DB_FILE, db_file) == false)
  {
    LOG_FAT << "Failed to read ORDER_DB_FILE";
    return -1;
  }
  ConcurrentSqliteDB order_db(db_file);

  if (EnvConfig::GetInstance()->GetValue(EnvVar::TRADE_DB_FILE, db_file) == false)
  {
    LOG_FAT << "Failed to read TRADE_DB_FILE";
    return -1;
  }
  ConcurrentSqliteDB trade_db(db_file);

  ProtoMessageDispatcher<ProtoMessagePtr> dispatcher;
  UserDB user_db(config_db, "UserInfo");
  bool ok = user_db.Initialize(dispatcher);

  ExchangeParameterDB exchange_db(config_db, "ExchangeParameter", "Holiday");
  ok = ok && exchange_db.Initialize(dispatcher);

  InstrumentDB instrument_db(config_db, "Instrument", exchange_db);
  ok = ok && instrument_db.Initialize(dispatcher);

  InterestRateDB interest_rate_db(config_db, "InterestRate");
  ok = ok && interest_rate_db.Initialize(dispatcher);

  SSRateDB ssrate_db(config_db, "SSRate", instrument_db, exchange_db);
  ok = ok && ssrate_db.Initialize(dispatcher);

  VolatilityCurveDB vol_curve_db(config_db, "VolatilityCurve", instrument_db, exchange_db);
  ok = ok && vol_curve_db.Initialize(dispatcher);

  CreditDB credit_db(config_db, "Credit", "CreditRecord", instrument_db, exchange_db);
  ok = ok && credit_db.Initialize(dispatcher);

  DestrikerDB destriker_db(config_db, "Destriker", instrument_db);
  ok = ok && destriker_db.Initialize(dispatcher);

  PositionDB position(config_db, "Position");
  ok = ok && position.Initialize(dispatcher);

  OrderDB order(order_db, instrument_db, exchange_db);
  ok = ok && order.Initialize(dispatcher);

  TradeDB trade(trade_db, instrument_db, exchange_db);
  ok = ok && trade.Initialize(dispatcher);

  PricerDB pricing(config_db, "Pricer", instrument_db);
  ok = ok && pricing.Initialize(dispatcher);

  StrategySwitchDB switches(config_db, "StrategySwitch", instrument_db);
  ok = ok && switches.Initialize(dispatcher);

  QuoterDB quoter(config_db, "Quoter", "QuoterRecord", instrument_db);
  ok = ok && quoter.Initialize(dispatcher);

  if (!ok)
  {
    LOG_ERR << "Failed to initialize DB!";
    return -1;
  }

  const int32_t capacity = 128;
  BlockingConcurrentQueue<std::tuple<google::protobuf::Message*, void*>> requests(capacity);
  std::thread rep_thread(ReplyRun, rep, std::ref(dispatcher), std::ref(requests));

  BlockingConcurrentQueue<ProtoMessagePtr> subscribes(capacity);
  std::thread sub_thread(SubscribeRun, sub, std::ref(dispatcher), std::ref(subscribes));

  auto *heartbeat = new Proto::Heartbeat();
  auto *info = new Proto::ServerInfo();
  auto *price = new Proto::Price();
  auto *cash = new Proto::Cash();
  // auto *pricing = new Proto::Pricer();
  while (true)
  {
    // nn_pollfd pfd[] = { {rep, NN_POLLIN, 0} };
    // nn_poll(pfd, 1, -1);
    nn_pollfd pfd[] = { {rep, NN_POLLIN, 0}, {sub, NN_POLLIN, 0} };
    nn_poll(pfd, 2, -1);

    if (pfd[0].revents & NN_POLLIN)
    {
      nn_msghdr hdr;
      memset(&hdr, 0, sizeof(nn_msghdr));
      void *body = NULL;
      nn_iovec iov;
      iov.iov_base = &body;
      iov.iov_len = NN_MSG;
      hdr.msg_iov = &iov;
      hdr.msg_iovlen = 1;
      void *control = NULL;
      hdr.msg_control = &control;
      hdr.msg_controllen = NN_MSG;
      int rc = nn_recvmsg(rep, &hdr, 0);
      if (rc >= 0)
      {
        LOG_INF << boost::format("Success to receive %1% bytes: %2%") % rc % GetString(body, rc);
        auto *msg = DecodeProtoMessage(body, rc);
        requests.enqueue(std::make_tuple(msg, control));
        nn_freemsg(body);
      }
      else
      {
        LOG_ERR << "Failed to receive request message: " << nn_strerror(nn_errno());
      }
    }
    else if (pfd[1].revents & NN_POLLIN)
    {
      void *buf = NULL;
      int bytes = nn_recv(sub, &buf, NN_MSG, 0);
      if (unlikely(bytes < 0))
      {
        LOG_ERR << "Failed to receive subsribe message: " << nn_strerror(nn_errno());
        continue;
      }
      auto *msg = DecodeProtoMessage(buf, bytes);
      if (msg)
      {
        LOG_INF << msg->GetTypeName() << " " << msg->ShortDebugString();
        subscribes.enqueue(ProtoMessagePtr(msg));
      }
      else
      {
        LOG_ERR << "Failed to decode proto message";
      }
      nn_freemsg(buf);
    }
  }

  nn_close(rep);
  nn_close(sub);

  delete heartbeat;
  delete info;
  delete price;
  delete cash;
  // delete pricing;
  return 0;
}
