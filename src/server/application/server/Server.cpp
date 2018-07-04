#include <mutex>
#include <thread>
#include <condition_variable>
#include <boost/format.hpp>
#include "base/common/Version.h"
#include "base/logger/Logging.h"
#include "Server.pb.h"
#include "config/EnvConfig.h"
#include "exchange/manager/ExchangeManager.h"
#include "strategy/ClusterManager.h"
#include "model/Middleware.h"
#include "model/ProductManager.h"
#include "model/ParameterManager.h"
#include "model/OrderManager.h"
#include "model/TradeManager.h"
#include "model/PositionManager.h"
#include "strategy/DeviceManager.h"
#include "strategy/StrategyDevice.h"

using namespace base;
using namespace std;

std::mutex mtx;
std::condition_variable cv;

int main(int argc, char *args[])
{
  Logger::InitFileLogger(EnvConfig::GetInstance()->GetString(EnvVar::LOGGING_DIR).c_str(),
                         EnvConfig::GetInstance()->GetString(EnvVar::APP_NAME).c_str(),
                         EnvConfig::GetInstance()->GetBool(EnvVar::ASYNC_LOGGING) == false);
  Logger::SetLogLevel(static_cast<Logger::LogLevel>(
        EnvConfig::GetInstance()->GetInt32(EnvVar::LOGGING_LEVEL)));
  Proto::Exchange exchange;
  Proto::Exchange_Parse(EnvConfig::GetInstance()->GetString(EnvVar::EXCHANGE), &exchange);
  Logger::SetNetOutput([&](Logger::LogLevel lvl, const char *data, int n)
      {
        auto info = Message::NewProto<Proto::ServerInfo>();
        info->set_exchange(exchange);
        info->set_info(data, n);
        info->set_type(static_cast<Proto::ServerInfo::Type>(lvl - Logger::PUBLISH));
        info->set_time(time(NULL));
        Middleware::GetInstance()->Publish(info);
      });
  LOG_INF << boost::format("Welcome BIRD-Server %1%.%2%.%3%, build date: %4%") %
    VER_MAJOR % VER_MINOR % VER_PATCH % __DATE__;

  Middleware::GetInstance()->Init();
  LOG_INF << "Middleware is running now";

  ClusterManager::GetInstance()->Init();
  ParameterManager::GetInstance()->InitGlobal();
  ExchangeManager::GetInstance()->Init();
  LOG_INF << "Exchange has been initialized";

  OrderManager::GetInstance()->Init();
  LOG_INF << "Order manager has been initialized";

  TradeManager::GetInstance()->Init();
  LOG_INF << "Trade manager has been initialized";

  PositionManager::GetInstance()->Init();
  LOG_INF << "Position manager has been initialized";

  ParameterManager::GetInstance()->Init();

  LOG_INF << "==================================================";
  LOG_PUB << "Initialization is done:)";
  LOG_INF << "==================================================";

  // ///// test
  // std::this_thread::sleep_for(std::chrono::seconds(3));
  // //// const Instrument* inst = ProductManager::GetInstance()->FindId("SR801");
  // const std::string id = "m1809";
  // const Instrument* inst = ProductManager::GetInstance()->FindId(id);
  // if (inst)
  // {
  //   auto *dm = ClusterManager::GetInstance()->FindDevice(inst);
  //   if (dm)
  //   {
  //     auto s = dm->FindStrategyDevice("m1809_P");
  //     if (s)
  //     {
  //       LOG_INF << "Start pricing strategy";
  //       s->Start();
  //       // std::this_thread::sleep_for(std::chrono::seconds(600));
  //       // s->Stop();
  //     }
  //     else
  //     {
  //       LOG_ERR << "Failed to find pricing strategy.";
  //     }
  //   }
  //   else
  //   {
  //     LOG_ERR << "Can't find device " << inst->Id();
  //   }
  // }
  // else
  // {
  //   LOG_ERR << "Can't find instrument " << id;
  // }
  //std::this_thread::sleep_for(std::chrono::seconds(60));
  //OrderManager::GetInstance()->Dump();
  ///// test

  std::unique_lock<std::mutex> lck(mtx);
  cv.wait(lck);
  return 0;
}
