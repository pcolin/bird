#include "InterestRateDB.h"
#include "base/logger/Logging.h"
#include "model/Message.h"

InterestRateDB::InterestRateDB(ConcurrentSqliteDB &db, const std::string &table_name)
    : DbBase(db, table_name) {}

void InterestRateDB::RefreshCache() {
  char query[1024];
  sprintf(query, "SELECT * FROM %s", table_name_.c_str());
  ExecSql(query, &cache_, &InterestRateDB::Callback);
}

void InterestRateDB::RegisterCallback(
    base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher) {
  dispatcher.RegisterCallback<Proto::InterestRateReq>(
      std::bind(&InterestRateDB::OnRequest, this, std::placeholders::_1));
}

base::ProtoMessagePtr InterestRateDB::OnRequest(const std::shared_ptr<Proto::InterestRateReq> &msg) {
  LOG_INF << "Interest rate request: " << msg->ShortDebugString();
  auto reply = std::make_shared<Proto::InterestRateRep>();
  if (msg->type() == Proto::RequestType::Get) {
    for (auto &it : cache_) {
      auto *r = reply->add_rates();
      r->set_days(it.first);
      r->set_rate(it.second);
    }
  } else if (msg->type() == Proto::RequestType::Set) {
    char sql[1024];
    TransactionGuard tg(this);
    sprintf(sql, "DELETE FROM %s", table_name_.c_str());
    ExecSql(sql);

    cache_.clear();
    for (auto &r : msg->rates()) {
      sprintf(sql, "INSERT INTO %s VALUES(%d, %f)", table_name_.c_str(), r.days(), r.rate());
      ExecSql(sql);
      cache_[r.days()] = r.rate();
    }
  } else if (msg->type() == Proto::RequestType::Del) {}
  reply->mutable_result()->set_result(true);
  return reply;
}

int InterestRateDB::Callback(void *data, int argc, char **argv, char **col_name) {
  auto &cache = *static_cast<InterestRateMap*>(data);
  cache[atoi(argv[0])] = atof(argv[1]);
  return 0;
}
