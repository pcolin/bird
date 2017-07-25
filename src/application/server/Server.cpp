#include <iostream>
#include <mutex>
#include <condition_variable>
#include <boost/format.hpp>
#include "base/common/Version.h"
#include "base/logger/Logging.h"
#include "exchange/manager/ExchangeManager.h"

using namespace base;
using namespace std;

std::mutex mtx;
std::condition_variable cv;

int main(int argc, char *args[])
{
  Logger::InitFileLogger("./", "BIRD", true);
  LOG_INF << boost::format("Welcome BIRD(Version %1%.%2%.%3%)") % VER_MAJOR % VER_MINOR % VER_PATCH;

  LOG_INF << "Middleware is running now";

  ExchangeManager::GetInstance()->Init();
  LOG_INF << "Exchange has been initialized";

  LOG_INF << "Initialization is done:)";

  std::unique_lock<std::mutex> lck(mtx);
  cv.wait(lck);
  return 0;
}
