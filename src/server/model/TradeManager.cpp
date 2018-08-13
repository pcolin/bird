#include "TradeManager.h"
#include "PositionManager.h"
#include "base/logger/Logging.h"
#include "base/common/Likely.h"

#include <boost/format.hpp>

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
  // LOG_INF << "OnTrade: " << trade->Dump();
  LOG_DBG << trade;
  const std::string exch_id = trade->id.substr(1);
  std::lock_guard<std::mutex> lck(mtx_);
  auto range = trades_.equal_range(exch_id);
  if (likely(range.first == range.second))
  {
    trades_.emplace(exch_id, trade);
    PositionManager::GetInstance()->OnTrade(trade);
  }
  else
  {
    bool same_inst = false;
    for (auto it = range.first; it != range.second; ++it)
    {
      if (it->second->instrument == trade->instrument)
      {
        if (it->second->side != trade->side)
        {
          LOG_ERR << boost::format("!!!ERROR: wash trade(%1%)!!!") % exch_id;
          PositionManager::GetInstance()->OnTrade(trade);
        }
        else
        {
          LOG_ERR << boost::format("Duplicate trade(%1%)") % trade->id;
        }
        same_inst = true;
      }
    }
    if (unlikely(!same_inst))
    {
      LOG_INF << boost::format("Trades with different instrument(%1%,%2%) have the same id(%3%)") %
        trade->instrument->Id() % range.first->second->instrument->Id() % exch_id;
      PositionManager::GetInstance()->OnTrade(trade);
    }
  }
}
