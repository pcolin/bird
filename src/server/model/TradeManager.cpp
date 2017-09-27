#include "TradeManager.h"
#include "base/logger/Logging.h"

TradeManager* TradeManager::GetInstance()
{
  static TradeManager manager;
  return &manager;
}

void TradeManager::Init()
{
  /// to be done...
}

void TradeManager::OnTrade(const TradePtr &trade)
{
  LOG_INF << "OnTrade " << trade->Dump();
}
