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

ExchangeManager::~ExchangeManager()
{
  if (trader_)
  {
    delete trader_;
    trader_ = nullptr;
  }
  if (md_)
  {
    delete md_;
    md_ = nullptr;
  }
}

void ExchangeManager::Init()
{
  LOG_INF << "Start Exchange API";
  trader_ = new CtpTraderApi();
  trader_->Init();

  md_ = new CtpMdApi();
  md_->Init();
}
