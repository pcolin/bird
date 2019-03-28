#include "MarketMakingStatisticDB.h"
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"
#include "model/Message.h"
#include "boost/format.hpp"

MarketMakingStatisticDB::MarketMakingStatisticDB(
    ConcurrentSqliteDB &db,
    const std::string &table_name,
    InstrumentDB &instrument_db,
    ProductParameterDB &product_db)
  : DbBase(db, table_name),
    instrument_db_(instrument_db),
    trading_day_(product_db.TradingDay()) {}

void MarketMakingStatisticDB::RefreshCache() {
  char sql[1024];
  sprintf(sql,
      "DELETE FROM %s WHERE NOT EXISTS(SELECT id FROM %s WHERE id = %s.underlying)",
      table_name_.c_str(), instrument_db_.TableName().c_str(), table_name_.c_str());
  ExecSql(sql);

  sprintf(sql, "SELECT * FROM %s WHERE date = %s",
      table_name_.c_str(), trading_day_.c_str());
  ExecSql(sql, &cache_, &MarketMakingStatisticDB::Callback);
}

void MarketMakingStatisticDB::RegisterCallback(
    base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher) {
  dispatcher.RegisterCallback<Proto::MarketMakingStatisticReq>(
      std::bind(&MarketMakingStatisticDB::OnRequest, this, std::placeholders::_1));
  dispatcher.RegisterCallback<Proto::MarketMakingStatistic>(
      std::bind(&MarketMakingStatisticDB::OnUpdate, this, std::placeholders::_1));
}

base::ProtoMessagePtr MarketMakingStatisticDB::OnRequest(
    const std::shared_ptr<Proto::MarketMakingStatisticReq> &msg) {
  LOG_INF << "MarketMakingStatistic request: " << msg->ShortDebugString();
  auto reply = std::make_shared<Proto::MarketMakingStatisticRep>();
  assert(msg->type() == Proto::RequestType::Get);
  for (auto &it : cache_) {
    reply->add_statistics()->CopyFrom(*it.second);
  }
  LOG_INF << boost::format("Get %1% statistics totally.") % reply->statistics_size();
  reply->mutable_result()->set_result(true);
  return reply;
}

base::ProtoMessagePtr MarketMakingStatisticDB::OnUpdate(
    const std::shared_ptr<Proto::MarketMakingStatistic> &msg) {
  LOG_INF << "MarketMakingStatistic: " << msg->ShortDebugString();
  char sql[1024];
  TransactionGuard tg(this);
  sprintf(sql, "INSERT OR REPLACE INTO %s VALUES('%s', '%s', %d, %d, %d, %d, %d, %d"
      ", %d, %d, %d, %d, %d, %d, %d, %f, %f)",
      table_name_.c_str(), msg->underlying().c_str(), msg->date().c_str(),
      static_cast<int32_t>(msg->exchange()), msg->orders(),
      msg->underlying_cancels(), msg->opening_quotes(),
      msg->closing_quotes(), msg->fuse_quotes(), msg->total_fuses(),
      msg->valid_qrs(), msg->total_qrs(), msg->quoting_options(),
      msg->valid_quotes(), msg->cum_valid_quotes(), msg->total_quotes(),
      msg->spread_ratio(), msg->cum_spread_ratio());
  if (ExecSql(sql)) {
    cache_[msg->underlying()] = msg;
  }
  return nullptr;
}

int MarketMakingStatisticDB::Callback(void *data, int argc, char **argv, char **col_name) {
  auto statistic = std::make_shared<Proto::MarketMakingStatistic>();
  std::string underlying = argv[0];
  statistic->set_underlying(underlying);
  statistic->set_date(argv[1]);
  statistic->set_exchange(static_cast<Proto::Exchange>(atoi(argv[2])));
  statistic->set_orders(atoi(argv[3]));
  statistic->set_underlying_cancels(atoi(argv[4]));
  statistic->set_opening_quotes(atoi(argv[5]));
  statistic->set_closing_quotes(atoi(argv[6]));
  statistic->set_fuse_quotes(atoi(argv[7]));
  statistic->set_total_fuses(atoi(argv[8]));
  statistic->set_valid_qrs(atoi(argv[9]));
  statistic->set_total_qrs(atoi(argv[10]));
  statistic->set_quoting_options(atoi(argv[11]));
  statistic->set_valid_quotes(atoi(argv[12]));
  statistic->set_cum_valid_quotes(atoi(argv[13]));
  statistic->set_total_quotes(atoi(argv[14]));
  statistic->set_spread_ratio(atof(argv[15]));
  statistic->set_cum_spread_ratio(atof(argv[16]));
  auto &cache = *static_cast<MarketMakingStatisticMap*>(data);
  cache[underlying] = statistic;
  return 0;
}
