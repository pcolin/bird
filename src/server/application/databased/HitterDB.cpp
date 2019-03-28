#include "HitterDB.h"
#include "base/logger/Logging.h"
#include "model/Message.h"
#include "boost/format.hpp"

HitterDB::HitterDB(ConcurrentSqliteDB &db,
                   const std::string &table_name,
                   const std::string &record_table_name,
                   InstrumentDB &instrument_db)
    : DbBase(db, table_name),
      record_table_name_(record_table_name),
      instrument_db_(instrument_db) {}

void HitterDB::RefreshCache() {
  char sql[1024];
  sprintf(sql, "DELETE FROM %s WHERE NOT EXISTS(SELECT id FROM %s WHERE id = %s.underlying)",
          table_name_.c_str(), instrument_db_.TableName().c_str(), table_name_.c_str());
  ExecSql(sql);
  sprintf(sql, "DELETE FROM %s WHERE NOT EXISTS(SELECT id FROM %s WHERE id = %s.instrument)",
          record_table_name_.c_str(),
          instrument_db_.TableName().c_str(),
          record_table_name_.c_str());
  ExecSql(sql);

  sprintf(sql, "SELECT * FROM %s", table_name_.c_str());
  auto data = std::make_tuple(&hitters_, &instrument_db_);
  ExecSql(sql, &data, &HitterDB::Callback);

  sprintf(sql, "SELECT * FROM %s", record_table_name_.c_str());
  ExecSql(sql, &data, &HitterDB::RecordCallback);
}

void HitterDB::RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher) {
  dispatcher.RegisterCallback<Proto::HitterReq>(
      std::bind(&HitterDB::OnRequest, this, std::placeholders::_1));
}

base::ProtoMessagePtr HitterDB::OnRequest(const std::shared_ptr<Proto::HitterReq> &msg) {
  LOG_INF << "Hitter request: " << msg->ShortDebugString();
  auto reply = std::make_shared<Proto::HitterRep>();
  if (msg->type() == Proto::RequestType::Get) {
    for (auto it : hitters_) {
      reply->add_hitters()->CopyFrom(*it.second);
    }
  } else if (msg->type() == Proto::RequestType::Set) {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &h : msg->hitters()) {
      const std::string &name = h.name();
      sprintf(sql, "INSERT OR REPLACE INTO %s VALUES('%s', '%s', '%s', %f, %d, %d, %d, "
          "%d, %d, %d)",
          table_name_.c_str(), name.c_str(), h.pricer().c_str(), h.underlying().c_str(),
          h.delta_limit(), h.order_limit(), h.trade_limit(), h.refill_limit(),
          h.bid_volume(), h.ask_volume(), h.max_volume());
      ExecSql(sql);
      auto it = hitters_.find(name);
      if (it != hitters_.end()) {
        it->second->CopyFrom(h);
        sprintf(sql, "DELETE FROM %s WHERE name='%s'",
            record_table_name_.c_str(), name.c_str());
        ExecSql(sql);
      } else {
        hitters_.emplace(name, std::make_shared<Proto::HitterSpec>(h));
      }

      for (auto &op : h.options()) {
        sprintf(sql, "INSERT INTO %s VALUES('%s', '%s')",
            record_table_name_.c_str(), name.c_str(), op.c_str());
        ExecSql(sql);
      }
    }
  } else if (msg->type() == Proto::RequestType::Del) {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &h : msg->hitters()) {
      const std::string &name = h.name();
      if (hitters_.erase(name) == 1) {
        sprintf(sql, "DELETE FROM %s WHERE name='%s'",
            table_name_.c_str(), name.c_str());
        ExecSql(sql);
        sprintf(sql, "DELETE FROM %s WHERE name='%s'",
            record_table_name_.c_str(), name.c_str());
        ExecSql(sql);
      }
    }
  }
  reply->mutable_result()->set_result(true);
  LOG_INF << "Hitter reply: " << reply->ShortDebugString();
  return reply;
}

int HitterDB::Callback(void *data, int argc, char **argv, char **col_name) {
  auto *tmp = static_cast<std::tuple<HitterMap*, InstrumentDB*>*>(data);
  auto *hitters = std::get<0>(*tmp);
  auto *instrument_db = std::get<1>(*tmp);

  const std::string underlying = argv[2];
  auto inst = instrument_db->FindUnderlying(underlying);
  if (inst) {
    auto hitter = std::make_shared<Proto::HitterSpec>();
    const std::string name = argv[0];
    hitter->set_name(name);
    hitter->set_pricer(argv[1]);
    hitter->set_underlying(underlying);
    hitter->set_delta_limit(atof(argv[3]));
    hitter->set_order_limit(atoi(argv[4]));
    hitter->set_trade_limit(atoi(argv[5]));
    hitter->set_refill_limit(atoi(argv[6]));
    hitter->set_bid_volume(atoi(argv[7]));
    hitter->set_ask_volume(atoi(argv[8]));
    hitter->set_max_volume(atoi(argv[9]));
    (*hitters)[name] = hitter;
  } else {
    LOG_ERR << underlying << " doesn't exist";
  }
  return 0;
}

int HitterDB::RecordCallback(void *data, int argc, char **argv, char **col_name) {
  auto *tmp = static_cast<std::tuple<HitterMap*, InstrumentDB*>*>(data);
  auto *hitters = std::get<0>(*tmp);
  auto *instrument_db = std::get<1>(*tmp);

  const std::string instrument = argv[1];
  auto option = instrument_db->FindOption(instrument);
  if (option) {
    auto it = hitters->find(argv[0]);
    if (it != hitters->end()) {
      it->second->add_options(std::move(instrument));
    }
  }
  return 0;
}
