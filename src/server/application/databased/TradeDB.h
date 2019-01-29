#ifndef DATABASED_TRADE_DB_H
#define DATABASED_TRADE_DB_H

#include <vector>
#include <thread>
#include <mutex>
#include "DbBase.h"
#include "InstrumentDB.h"
#include "ProductParameterDB.h"
#include "base/concurrency/blockingconcurrentqueue.h"
#include "Trade.pb.h"

class TradeDB : public DbBase {
  typedef std::vector<std::shared_ptr<Proto::Trade>> TradeArray;

 public:
  TradeDB(ConcurrentSqliteDB &db, InstrumentDB &instrument_db, ProductParameterDB &product_db);

 private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  base::ProtoMessagePtr OnRequest(const std::shared_ptr<Proto::TradeReq> &msg);
  base::ProtoMessagePtr OnTrade(const std::shared_ptr<Proto::Trade> &msg);
  // void UpdateTrade(const Proto::Trade &inst, TradeMap &cache);

  void Run();

  static int Callback(void *data, int argc, char **argv, char **col_name);

  TradeArray trades_;
  std::mutex mtx_;

  const int kCapacity = 128;
  moodycamel::BlockingConcurrentQueue<std::shared_ptr<Proto::Trade>> requests_;
  std::thread thread_;
  InstrumentDB &instrument_db_;
};

#endif // DATABASED_TRADE_DB_H
