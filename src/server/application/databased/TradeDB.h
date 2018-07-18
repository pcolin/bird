#ifndef DATABASED_TRADE_DB_H
#define DATABASED_TRADE_DB_H

#include "DbBase.h"
#include "InstrumentDB.h"
#include "ExchangeParameterDB.h"
#include "base/concurrency/blockingconcurrentqueue.h"
#include "Trade.pb.h"

#include <unordered_map>
#include <thread>
#include <mutex>

class TradeDB : public DbBase
{
  typedef std::unordered_map<std::string, std::shared_ptr<Proto::Trade>> TradeMap;
public:
  TradeDB(ConcurrentSqliteDB &db, InstrumentDB &instrument_db, ExchangeParameterDB &exchange_db);

private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  base::ProtoMessagePtr OnRequest(const std::shared_ptr<Proto::TradeReq> &msg);
  void UpdateTrade(const Proto::Trade &inst, TradeMap &cache);

  void Run();

  static int Callback(void *data, int argc, char **argv, char **col_name);

  TradeMap trades_;
  std::mutex mtx_;

  const int capacity_ = 128;
  moodycamel::BlockingConcurrentQueue<std::shared_ptr<Proto::TradeReq>> requests_;
  std::thread thread_;
  InstrumentDB &instrument_db_;
};

#endif
