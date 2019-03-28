#include "SSRateDB.h"
#include "base/logger/Logging.h"
#include "model/Message.h"
#include "boost/format.hpp"

SSRateDB::SSRateDB(ConcurrentSqliteDB &db,
                   const std::string &table_name,
                   InstrumentDB &instrument_db,
                   ProductParameterDB &product_db)
    : DbBase(db, table_name),
      instrument_db_(instrument_db),
      trading_day_(product_db.TradingDay()) {}

void SSRateDB::RefreshCache() {
  char sql[1024];
  sprintf(sql, "DELETE FROM %s WHERE maturity<'%s'", table_name_.c_str(), trading_day_.c_str());
  ExecSql(sql);

  sprintf(sql, "SELECT * FROM %s", table_name_.c_str());
  auto data = std::make_tuple(&cache_, &instrument_db_);
  ExecSql(sql, &data, &SSRateDB::Callback);
}

void SSRateDB::RegisterCallback(
    base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher) {
  dispatcher.RegisterCallback<Proto::SSRateReq>(
      std::bind(&SSRateDB::OnRequest, this, std::placeholders::_1));
}

base::ProtoMessagePtr SSRateDB::OnRequest(const std::shared_ptr<Proto::SSRateReq> &msg) {
  LOG_INF << "SSRate request: " << msg->ShortDebugString();
  auto reply = std::make_shared<Proto::SSRateRep>();
  Proto::RequestType type = msg->type();
  if (type == Proto::RequestType::Get) {
    if (msg->underlying().empty()) {
      for (auto &ssr : cache_) {
        for (auto &r : ssr.second) {
          auto *tmp = reply->add_rates();
          tmp->set_underlying(ssr.first);
          tmp->set_maturity(r.first);
          tmp->set_rate(r.second);
        }
      }
    } else {
      auto it = cache_.find(msg->underlying());
      if (it != cache_.end()) {
        for (auto &r : it->second) {
          auto *tmp = reply->add_rates();
          tmp->set_underlying(it->first);
          tmp->set_maturity(r.first);
          tmp->set_rate(r.second);
        }
      } else {
        reply->mutable_result()->set_result(false);
        reply->mutable_result()->set_error(msg->underlying() + " doesn't exist");
        return reply;
      }
    }
    LOG_INF << boost::format("Get %1% ssrates totally.") % reply->rates_size();
  } else if (type == Proto::RequestType::Set) {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &r : msg->rates()) {
      sprintf(sql, "INSERT OR REPLACE INTO %s VALUES('%s', '%s', %f)", table_name_.c_str(),
              r.underlying().c_str(), r.maturity().c_str(), r.rate());
      ExecSql(sql);
      cache_[r.underlying()][r.maturity()] = r.rate();
    }
  } else if (type == Proto::RequestType::Del) {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &r : msg->rates()) {
      sprintf(sql, "DELETE FROM %s WHERE underlying='%s'AND maturity='%s'", table_name_.c_str(),
              r.underlying().c_str(), r.maturity().c_str());
      ExecSql(sql);
      auto it = cache_.find(r.underlying());
      if (it != cache_.end()) {
        it->second.erase(r.maturity());
      }
    }
  }
  reply->mutable_result()->set_result(true);
  return reply;
}

int SSRateDB::Callback(void *data, int argc, char **argv, char **col_name) {
  assert(argc == 3);
  auto *tmp = static_cast<std::tuple<SSRateMap*, InstrumentDB*>*>(data);
  auto *cache = std::get<0>(*tmp);
  auto *instrument_db = std::get<1>(*tmp);

  const std::string underlying = argv[0];
  auto inst = instrument_db->FindUnderlying(underlying);
  if (inst) {
    (*cache)[underlying][argv[1]] = atof(argv[2]);
  } else {
    LOG_ERR << "Failed to find underlying " << underlying;
  }
  return 0;
}
