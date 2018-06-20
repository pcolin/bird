#include "base/common/Version.h"
#include "base/common/ProtoMessageCoder.h"
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"
#include "Heartbeat.pb.h"
#include "Server.pb.h"
#include "Instrument.pb.h"
#include "Price.pb.h"
#include "Order.pb.h"
#include "Trade.pb.h"
#include "Cash.pb.h"
#include "Position.pb.h"
#include "ExchangeParameter.pb.h"
#include "SSRate.pb.h"
#include "InterestRate.pb.h"
#include "Volatility.pb.h"
#include "Destriker.pb.h"
#include "Pricer.pb.h"
#include "Strategy.pb.h"

#include "nn.h"
#include "pubsub.h"

#include <thread>
#include <boost/format.hpp>

using namespace base;

int main(int argc, char *argv[])
{
  Logger::InitFileLogger(EnvConfig::GetInstance()->GetString(EnvVar::LOGGING_DIR).c_str(),
                         EnvConfig::GetInstance()->GetString(EnvVar::APP_NAME).c_str(),
                         EnvConfig::GetInstance()->GetBool(EnvVar::ASYNC_LOGGING) == false);
  Logger::SetLogLevel(static_cast<Logger::LogLevel>(
        EnvConfig::GetInstance()->GetInt32(EnvVar::LOGGING_LEVEL)));
  LOG_INF << boost::format("Welcome BIRD-Proxy %1%.%2%.%3%") % VER_MAJOR % VER_MINOR % VER_PATCH;

  int pub = nn_socket(AF_SP, NN_PUB);
  if (pub < 0)
  {
    LOG_ERR << "Failed to create pub socket: " << nn_strerror(nn_errno());
    return -1;
  }
  const std::string pub_addr = EnvConfig::GetInstance()->GetString(EnvVar::BIND_SUB_ADDR);
  if (nn_bind(pub, pub_addr.c_str()) < 0)
  {
    LOG_ERR << "Failed to bind pub socket: " << nn_strerror(nn_errno());
    nn_close(pub);
    return -1;
  }

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

  auto *heartbeat = new Proto::Heartbeat();
  auto *info = new Proto::ServerInfo();
  auto *instrument = new Proto::InstrumentReq();
  auto *price = new Proto::Price();
  auto *order = new Proto::Order();
  auto *trade = new Proto::Trade();
  auto *cash = new Proto::Cash();
  auto *position = new Proto::PositionReq();
  auto *exchange_parameter = new Proto::ExchangeParameterReq();
  auto *interest_rate = new Proto::InterestRateReq();
  auto *ssrate = new Proto::SSRateReq();
  auto *volatility = new Proto::VolatilityCurveReq();
  auto *destriker = new Proto::DestrikerReq();
  auto *pricer = new Proto::PricerReq();
  auto *strategy = new Proto::StrategyStatus();

  int32_t serial_num = 0;
  while (true)
  {
    void *buf = NULL;
    int recv_bytes = nn_recv(sub, &buf, NN_MSG, 0);
    if (unlikely(recv_bytes < 0))
    {
      LOG_ERR << "Failed to receive message: " << nn_strerror(nn_errno());
      continue;
    }
    auto *msg = DecodeProtoMessage(buf, recv_bytes);
    if (msg)
    {
      LOG_INF << boost::format("%1% %2% %3%") % serial_num % msg->GetTypeName() %
        msg->ShortDebugString();
      delete msg;
      int32_t tmp = htonl(serial_num++);
      memcpy(buf, reinterpret_cast<char*>(&tmp), sizeof(tmp));
      int send_bytes  = nn_send(pub, buf, recv_bytes, 0);
      if (unlikely(send_bytes != recv_bytes))
      {
        LOG_ERR << boost::format("Failed to publish message(%1% != %2%): %3%") % recv_bytes %
          send_bytes % nn_strerror(nn_errno());
      }
    }
    else
    {
      LOG_ERR << "Failed to decode proto message: " << GetString(buf, recv_bytes);
    }
    nn_freemsg(buf);
  }
  delete heartbeat;
  delete info;
  delete instrument;
  delete price;
  delete order;
  delete trade;
  delete cash;
  delete position;
  delete exchange_parameter;
  delete ssrate;
  delete interest_rate;
  delete volatility;
  delete destriker;
  delete pricer;
  delete strategy;
  return 0;
}
