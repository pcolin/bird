#ifndef MODEL_TRADE_MANAGER_H
#define MODEL_TRADE_MANAGER_H

#include "Trade.h"

#include <unordered_map>
#include <mutex>

class TradeManager
{
public:
  static TradeManager* GetInstance();
  ~TradeManager() {}

  void Init();
  void OnTrade(const TradePtr &trade);

private:
  TradeManager() {}

  typedef std::unordered_multimap<std::string, TradePtr> TradeMap;
  TradeMap trades_;
  std::mutex mtx_;
};

#endif
