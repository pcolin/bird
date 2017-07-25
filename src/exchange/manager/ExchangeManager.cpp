#include "ExchangeManager.h"
#include "../ctp/CtpTraderApi.h"
#include "../ctp/CtpMdApi.h"
#include "base/logger/Logging.h"

using namespace base;

ExchangeManager* ExchangeManager::GetInstance()
{
  static ExchangeManager manager;
  return &manager;
}

void ExchangeManager::Init()
{
  LOG_INF << "Start Exchange API";
  trader_ = std::make_shared<CtpTraderApi>();
  trader_->Init();

  md_ = std::make_shared<CtpMdApi>();
  md_->Init();
}
