#include "DestrikerDB.h"
#include "base/logger/Logging.h"
#include "model/Message.h"

DestrikerDB::DestrikerDB(ConcurrentSqliteDB &db,
                         const std::string &table_name,
                         InstrumentDB &instrument_db)
    : DbBase(db, table_name), instrument_db_(instrument_db) {}

void DestrikerDB::RefreshCache() {
  char sql[1024];
  sprintf(sql, "DELETE FROM %s WHERE NOT EXISTS(SELECT id FROM %s WHERE id = %s.instrument)",
          table_name_.c_str(), instrument_db_.TableName().c_str(), table_name_.c_str());
  ExecSql(sql);

  sprintf(sql, "SELECT * FROM %s", table_name_.c_str());
  auto data = std::make_tuple(&cache_, &instrument_db_);
  ExecSql(sql, &data, &DestrikerDB::Callback);
}

void DestrikerDB::RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher) {
  dispatcher.RegisterCallback<Proto::DestrikerReq>(
      std::bind(&DestrikerDB::OnRequest, this, std::placeholders::_1));
}

base::ProtoMessagePtr DestrikerDB::OnRequest(const std::shared_ptr<Proto::DestrikerReq> &msg) {
  LOG_INF << "Destriker request: " << msg->ShortDebugString();
  auto reply = Message<Proto::DestrikerRep>::New();
  Proto::RequestType type = msg->type();
  if (type == Proto::RequestType::Get) {
    for (auto &it : cache_) {
      auto inst = instrument_db_.FindOption(it.first);
      if (inst) {
        auto *d = reply->add_destrikers();
        d->set_instrument(it.first);
        d->set_destriker(it.second);
      }
    }
  } else if (type == Proto::RequestType::Set) {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &d : msg->destrikers()) {
      sprintf(sql, "INSERT OR REPLACE INTO %s VALUES('%s', %f)",
              table_name_.c_str(), d.instrument().c_str(), d.destriker());
      ExecSql(sql);
      cache_[d.instrument()] = d.destriker();
    }
  } else if (type == Proto::RequestType::Del) {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &d : msg->destrikers()) {
      sprintf(sql, "DELETE FROM %s WHERE instrument='%s'", table_name_.c_str(),
              d.instrument().c_str());
      ExecSql(sql);
      cache_.erase(d.instrument());
    }
  }
  reply->mutable_result()->set_result(true);
  return reply;
}

int DestrikerDB::Callback(void *data, int argc, char **argv, char **col_name) {
  assert(argc == 2);
  auto *tmp = static_cast<std::tuple<DestrikerMap*, InstrumentDB*>*>(data);
  auto *destrikers = std::get<0>(*tmp);
  auto *instrument_db = std::get<1>(*tmp);

  const std::string id = argv[0];
  auto inst = instrument_db->FindOption(id);
  if (inst) {
    (*destrikers)[id] = atof(argv[1]);
  }
}
