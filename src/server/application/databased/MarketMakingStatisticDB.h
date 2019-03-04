#ifndef DATABASED_MARKET_MAKING_STATISTIC_DB_H
#define DATABASED_MARKET_MAKING_STATISTIC_DB_H

#include <unordered_map>
#include "DbBase.h"
#include "InstrumentDB.h"
#include "base/concurrency/blockingconcurrentqueue.h"
#include "MarketMakingStatistic.pb.h"

class MarketMakingStatisticDB : public DbBase {
 public:
  MarketMakingStatisticDB(ConcurrentSqliteDB &db, const std::string &table_name,
      InstrumentDB &instrument_db, ProductParameterDB &product_db);

 private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(
      base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher) override;

  base::ProtoMessagePtr OnRequest(
      const std::shared_ptr<Proto::MarketMakingStatisticReq> &msg);
  base::ProtoMessagePtr OnUpdate(
      const std::shared_ptr<Proto::MarketMakingStatistic> &msg);

  static int Callback(void *data, int argc, char **argv, char **col_name);

  typedef std::unordered_map<std::string,
          std::shared_ptr<Proto::MarketMakingStatistic>> MarketMakingStatisticMap;
  MarketMakingStatisticMap cache_;
  InstrumentDB &instrument_db_;
  std::string trading_day_;
};

#endif // DATABASED_MARKET_MAKING_STATISTIC_DB_H
