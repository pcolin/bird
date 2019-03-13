#include "DimerDB.h"
#include "base/logger/Logging.h"
#include "model/Message.h"
#include "boost/format.hpp"

DimerDB::DimerDB(ConcurrentSqliteDB &db,
                   const std::string &table_name,
                   const std::string &record_table_name,
                   InstrumentDB &instrument_db)
    : DbBase(db, table_name),
      record_table_name_(record_table_name),
      instrument_db_(instrument_db) {}

void DimerDB::RefreshCache() {
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
  auto data = std::make_tuple(&dimers_, &instrument_db_);
  ExecSql(sql, &data, &DimerDB::Callback);

  sprintf(sql, "SELECT * FROM %s", record_table_name_.c_str());
  ExecSql(sql, &data, &DimerDB::RecordCallback);
}

void DimerDB::RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher) {
  dispatcher.RegisterCallback<Proto::DimerReq>(
      std::bind(&DimerDB::OnRequest, this, std::placeholders::_1));
}

base::ProtoMessagePtr DimerDB::OnRequest(const std::shared_ptr<Proto::DimerReq> &msg) {
  LOG_INF << "Dimer request: " << msg->ShortDebugString();
  auto reply = Message<Proto::DimerRep>::New();
  if (msg->type() == Proto::RequestType::Get) {
    for (auto &it : dimers_) {
      reply->add_dimers()->CopyFrom(*it.second);
    }
  } else if (msg->type() == Proto::RequestType::Set) {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &d : msg->dimers()) {
      const std::string &name = d.name();
      sprintf(sql, "INSERT OR REPLACE INTO %s VALUES('%s', '%s', '%s', %f, %d, %d, %d, "
          "%d, %d, %d)",
          table_name_.c_str(), name.c_str(), d.pricer().c_str(), d.underlying().c_str(),
          d.delta_limit(), d.order_limit(), d.trade_limit(), d.refill_limit(),
          d.bid_volume(), d.ask_volume(), d.tick());
      ExecSql(sql);
      auto it = dimers_.find(name);
      if (it != dimers_.end()) {
        it->second->CopyFrom(d);
        sprintf(sql, "DELETE FROM %s WHERE name='%s'",
            record_table_name_.c_str(), name.c_str());
        ExecSql(sql);
      } else {
        dimers_.emplace(name, Message<Proto::DimerSpec>::New(d));
      }

      for (auto &op : d.options()) {
        sprintf(sql, "INSERT INTO %s VALUES('%s', '%s')",
            record_table_name_.c_str(), name.c_str(), op.c_str());
        ExecSql(sql);
      }
    }
  } else if (msg->type() == Proto::RequestType::Del) {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &d : msg->dimers()) {
      const std::string &name = d.name();
      if (dimers_.erase(name) == 1) {
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
  LOG_INF << "Dimer reply: " << reply->ShortDebugString();
  return reply;
}

int DimerDB::Callback(void *data, int argc, char **argv, char **col_name) {
  auto *tmp = static_cast<std::tuple<DimerMap*, InstrumentDB*>*>(data);
  auto *dimers = std::get<0>(*tmp);
  auto *instrument_db = std::get<1>(*tmp);

  const std::string underlying = argv[2];
  auto inst = instrument_db->FindUnderlying(underlying);
  if (inst) {
    auto dimer = Message<Proto::DimerSpec>::New();
    const std::string name = argv[0];
    dimer->set_name(name);
    dimer->set_pricer(argv[1]);
    dimer->set_underlying(underlying);
    dimer->set_delta_limit(atof(argv[3]));
    dimer->set_order_limit(atoi(argv[4]));
    dimer->set_trade_limit(atoi(argv[5]));
    dimer->set_refill_limit(atoi(argv[6]));
    dimer->set_bid_volume(atoi(argv[7]));
    dimer->set_ask_volume(atoi(argv[8]));
    dimer->set_tick(atoi(argv[9]));
    (*dimers)[name] = dimer;
  } else {
    LOG_ERR << underlying << " doesn't exist";
  }
  return 0;
}

int DimerDB::RecordCallback(void *data, int argc, char **argv, char **col_name) {
  auto *tmp = static_cast<std::tuple<DimerMap*, InstrumentDB*>*>(data);
  auto *dimers = std::get<0>(*tmp);
  auto *instrument_db = std::get<1>(*tmp);

  const std::string instrument = argv[1];
  auto option = instrument_db->FindOption(instrument);
  if (option) {
    auto it = dimers->find(argv[0]);
    if (it != dimers->end()) {
      it->second->add_options(std::move(instrument));
    }
  }
  return 0;
}
