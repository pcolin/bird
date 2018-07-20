#ifndef DATABASED_ORDER_DB_H
#define DATABASED_ORDER_DB_H

#include "DbBase.h"
#include "InstrumentDB.h"
#include "ExchangeParameterDB.h"
#include "base/concurrency/blockingconcurrentqueue.h"
#include "Order.pb.h"

#include <unordered_map>
#include <thread>
#include <mutex>

class OrderDB : public DbBase
{
  typedef std::unordered_map<size_t, std::shared_ptr<Proto::Order>> OrderMap;
public:
  OrderDB(ConcurrentSqliteDB &db, InstrumentDB &instrument_db, ExchangeParameterDB &exchange_db);

private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  base::ProtoMessagePtr OnRequest(const std::shared_ptr<Proto::OrderReq> &msg);
  // void UpdateOrder(const Proto::Order &inst, OrderMap &cache);

  void Run();

  static int Callback(void *data, int argc, char **argv, char **col_name);

  OrderMap orders_;
  std::mutex mtx_;

  const int capacity_ = 128;
  moodycamel::BlockingConcurrentQueue<std::shared_ptr<Proto::OrderReq>> requests_;
  std::thread thread_;
  InstrumentDB &instrument_db_;
};

#endif
