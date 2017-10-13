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
  Logger::SetNetOutput([](Logger::LogLevel lvl, const char *data, int n)
      {
        auto info = Message::NewProto<proto::ServerInfo>();
        info->set_info(data, n);
        info->set_type(static_cast<proto::ServerInfo::Type>(lvl - Logger::PUBLISH));
        info->set_time(time(NULL));
        Middleware::GetInstance()->Publish(info);
      });
  LOG_INF << boost::format("Welcome BIRD-Server %1%.%2%.%3%") % VER_MAJOR % VER_MINOR % VER_PATCH;

  Middleware::GetInstance()->Init();
  LOG_INF << "Middleware is running now";

  ClusterManager::GetInstance()->Init();

  ExchangeManager::GetInstance()->Init();
  LOG_INF << "Exchange has been initialized";

  OrderManager::GetInstance()->Init();
  LOG_INF << "Order manager has been initialized";

  TradeManager::GetInstance()->Init();
  LOG_INF << "Trade manager has been initialized";

  PositionManager::GetInstance()->Init();
  LOG_INF << "Position manager has been initialized";

  /// to be done...
  // ParamManager::GetInstance()->Init();

  LOG_PUB << "Initialization is done:)";

  ///// test
  //std::this_thread::sleep_for(std::chrono::seconds(30));
  //// const Instrument* inst = ProductManager::GetInstance()->FindId("SR801");
  //const Instrument* inst = ProductManager::GetInstance()->FindId("m1801");
  //if (inst)
  //{
  //  auto *dm = ClusterManager::GetInstance()->FindDevice(inst);
  //  if (dm)
  //  {
  //    auto s = dm->FindStrategyDevice("test");
  //    if (s)
  //    {
  //      LOG_INF << "Start test strategy";
  //      s->Start();
  //      std::this_thread::sleep_for(std::chrono::seconds(60));
  //      s->Stop();
  //    }
  //    else
  //    {
  //      LOG_ERR << "Failed to find test strategy.";
  //    }
  //  }
  //}
  //std::this_thread::sleep_for(std::chrono::seconds(60));
  //OrderManager::GetInstance()->Dump();
  ///// test

  std::unique_lock<std::mutex> lck(mtx);
  cv.wait(lck);
  return 0;
}
