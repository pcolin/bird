#include "HedgerDB.h"
#include "base/logger/Logging.h"
#include "model/Message.h"
#include "boost/format.hpp"

HedgerDB::HedgerDB(
    ConcurrentSqliteDB &db,
    const std::string &table_name,
    InstrumentDB &instrument_db)
    : DbBase(db, table_name),
      instrument_db_(instrument_db) {}

void HedgerDB::RefreshCache() {
  char sql[1024];
  sprintf(sql, "DELETE FROM %s WHERE NOT EXISTS(SELECT id FROM %s WHERE id = %s.underlying)",
          table_name_.c_str(), instrument_db_.TableName().c_str(), table_name_.c_str());
  ExecSql(sql);

  sprintf(sql, "SELECT * FROM %s", table_name_.c_str());
  auto data = std::make_tuple(&hedgers_, &instrument_db_);
  ExecSql(sql, &data, &HedgerDB::Callback);
}

void HedgerDB::RegisterCallback(
    base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher) {
  dispatcher.RegisterCallback<Proto::HedgerReq>(
      std::bind(&HedgerDB::OnRequest, this, std::placeholders::_1));
}

base::ProtoMessagePtr HedgerDB::OnRequest(const std::shared_ptr<Proto::HedgerReq> &msg) {
  LOG_INF << "Hedger request: " << msg->ShortDebugString();
  auto reply = Message<Proto::HedgerRep>::New();
  // if (msg->type() == Proto::RequestType::Get) {
  //   for (auto &it : hedgers_) {
  //     reply->add_hedger()->CopyFrom(*it.second);
  //   }
  // } else if (msg->type() == Proto::RequestType::Set) {
  //   char sql[1024];
  //   TransactionGuard tg(this);
  //   for (auto &d : msg->hedger()) {
  //     const std::string &name = d.name();
  //     sprintf(sql, "INSERT OR REPLACE INTO %s VALUES('%s', '%s', '%s', %f, %d, %d, %d, "
  //         "%d, %d, %d)",
  //         table_name_.c_str(), name.c_str(), d.pricer().c_str(), d.underlying().c_str(),
  //         d.delta_limit(), d.order_limit(), d.trade_limit(), d.refill_limit(),
  //         d.bid_volume(), d.ask_volume(), d.tick());
  //     ExecSql(sql);
  //     auto it = hedger_.find(name);
  //     if (it != hedger_.end()) {
  //       it->second->CopyFrom(d);
  //       sprintf(sql, "DELETE FROM %s WHERE name='%s'",
  //           record_table_name_.c_str(), name.c_str());
  //       ExecSql(sql);
  //     } else {
  //       hedger_.emplace(name, Message<Proto::HedgerSpec>::New(d));
  //     }

  //     for (auto &op : d.options()) {
  //       sprintf(sql, "INSERT INTO %s VALUES('%s', '%s')",
  //           record_table_name_.c_str(), name.c_str(), op.c_str());
  //       ExecSql(sql);
  //     }
  //   }
  // } else if (msg->type() == Proto::RequestType::Del) {
  //   char sql[1024];
  //   TransactionGuard tg(this);
  //   for (auto &d : msg->hedger()) {
  //     const std::string &name = d.name();
  //     if (hedger_.erase(name) == 1) {
  //       sprintf(sql, "DELETE FROM %s WHERE name='%s'",
  //           table_name_.c_str(), name.c_str());
  //       ExecSql(sql);
  //       sprintf(sql, "DELETE FROM %s WHERE name='%s'",
  //           record_table_name_.c_str(), name.c_str());
  //       ExecSql(sql);
  //     }
  //   }
  // }
  reply->mutable_result()->set_result(true);
  LOG_INF << "Hedger reply: " << reply->ShortDebugString();
  return reply;
}

int HedgerDB::Callback(void *data, int argc, char **argv, char **col_name) {
  auto *tmp = static_cast<std::tuple<HedgerMap*, InstrumentDB*>*>(data);
  auto *hedger = std::get<0>(*tmp);
  auto *instrument_db = std::get<1>(*tmp);

  // const std::string underlying = argv[2];
  // auto inst = instrument_db->FindUnderlying(underlying);
  // if (inst) {
  //   auto hedger = Message<Proto::HedgerSpec>::New();
  //   const std::string name = argv[0];
  //   hedger->set_name(name);
  //   hedger->set_pricer(argv[1]);
  //   hedger->set_underlying(underlying);
  //   hedger->set_delta_limit(atof(argv[3]));
  //   hedger->set_order_limit(atoi(argv[4]));
  //   hedger->set_trade_limit(atoi(argv[5]));
  //   hedger->set_refill_limit(atoi(argv[6]));
  //   hedger->set_bid_volume(atoi(argv[7]));
  //   hedger->set_ask_volume(atoi(argv[8]));
  //   hedger->set_tick(atoi(argv[9]));
  //   (*hedger)[name] = hedger;
  // } else {
  //   LOG_ERR << underlying << " doesn't exist";
  // }
  return 0;
}
