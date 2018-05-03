#include "ExchangeManager.h"
#include "../simulation/SimulationTraderApi.h"
#include "../simulation/SimulationMdApi.h"
#include "../ctp/CtpTraderApi.h"
#include "../ctp/CtpMdApi.h"
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"

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
  bool simulated = EnvConfig::GetInstance()->GetBool(EnvVar::SIMULATED_EXCHANGE, false);
  if (simulated)
  {
    LOG_INF << "Start Simualtion exchange api";
    trader_ = new SimulationTraderApi();
    trader_->Init();

    md_ = new SimulationMdApi();
    md_->Init();
  }
  else
  {
    LOG_INF << "Start CTP exchange api";
    trader_ = new CtpTraderApi();
    trader_->Init();

    md_ = new CtpMdApi();
    md_->Init();
  }
}
