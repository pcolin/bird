#include "CreditDB.h"
#include "base/logger/Logging.h"
#include "model/Message.h"
#include "boost/format.hpp"

CreditDB::CreditDB(ConcurrentSqliteDB &db,
                   const std::string &table_name,
                   const std::string &record_table_name,
                   InstrumentDB &instrument_db,
                   ProductParameterDB &product_db)
    : DbBase(db, table_name),
      caches_(Proto::StrategyType::DummyQuoter),
      record_table_name_(record_table_name),
      instrument_db_(instrument_db),
      trading_day_(product_db.TradingDay()) {}

void CreditDB::RefreshCache() {
  char sql[1024];
  sprintf(sql, "DELETE FROM %s WHERE maturity<'%s'", table_name_.c_str(), trading_day_.c_str());
  ExecSql(sql);
  sprintf(sql, "DELETE FROM %s WHERE NOT EXISTS(SELECT id FROM %s WHERE id = %s.option)",
      record_table_name_.c_str(), instrument_db_.TableName().c_str(), record_table_name_.c_str());
  ExecSql(sql);

  sprintf(sql, "SELECT * FROM %s", table_name_.c_str());
  auto data = std::make_tuple(&caches_, &instrument_db_);
  ExecSql(sql, &data, &CreditDB::Callback);
  sprintf(sql, "SELECT * FROM %s", record_table_name_.c_str());
  ExecSql(sql, &data, &CreditDB::RecordCallback);
}

void CreditDB::RegisterCallback(
    base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher) {
  dispatcher.RegisterCallback<Proto::CreditReq>(
      std::bind(&CreditDB::OnRequest, this, std::placeholders::_1));
}

base::ProtoMessagePtr CreditDB::OnRequest(const std::shared_ptr<Proto::CreditReq> &msg) {
  LOG_INF << "Credit request: " << msg->ShortDebugString();
  auto reply = Message<Proto::CreditRep>::New();
  Proto::RequestType type = msg->type();
  if (type == Proto::RequestType::Get) {
    for (auto &cache: caches_) {
      for (auto &credit : cache) {
        for (auto &c : credit.second) {
          reply->add_credits()->CopyFrom(*c.second);
        }
      }
    }
    LOG_INF << boost::format("Get %1% credits totally.") % reply->credits_size();
  } else if (type == Proto::RequestType::Set) {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &c : msg->credits()) {
      const std::string &m = c.maturity();
      sprintf(sql, "INSERT OR REPLACE INTO %s VALUES(%d, '%s', '%s', %f, %f, %f, %f, %f, %f, %f)",
              table_name_.c_str(), static_cast<int>(c.strategy()), c.underlying().c_str(), m.c_str(),
              c.delta(), c.vega(), c.skew(), c.convex(), c.cash(), c.price(), c.multiplier());
      ExecSql(sql);
      auto &credits = caches_[c.strategy()][c.underlying()];
      auto it = credits.find(m);
      if (it != credits.end()) {
        it->second->CopyFrom(c);
        sprintf(sql, "DELETE FROM %s WHERE strategy=%d AND EXISTS"
                     "(SELECT id FROM '%s' WHERE hedge_underlying='%s' AND maturity='%s')",
                record_table_name_.c_str(), c.strategy(), instrument_db_.TableName().c_str(),
                c.underlying().c_str(), m.c_str());
        ExecSql(sql);
      } else {
        // auto credit = Message::NewProto<Proto::Credit>();
        // credit->CopyFrom(c);
        credits.emplace(m, Message<Proto::Credit>::New(c));
      }

      for (auto &r : c.records()) {
        sprintf(sql, "INSERT INTO '%s' VALUES(%d, '%s', %f)", record_table_name_.c_str(),
                c.strategy(), r.option().c_str(), r.credit());
        ExecSql(sql);
      }
    }
  }
  else if (type == Proto::RequestType::Del) { }
  reply->mutable_result()->set_result(true);
  return reply;
}

int CreditDB::Callback(void *data, int argc, char **argv, char **col_name) {
  assert(argc == 10);
  auto *tmp = static_cast<std::tuple<std::vector<CreditMap>*, InstrumentDB*>*>(data);
  auto *caches = std::get<0>(*tmp);
  auto *instrument_db = std::get<1>(*tmp);

  std::string underlying = argv[1];
  auto inst = instrument_db->FindUnderlying(underlying);
  if (inst) {
    auto c = Message<Proto::Credit>::New();
    auto strategy = static_cast<Proto::StrategyType>(atoi(argv[0]));
    c->set_strategy(strategy);
    c->set_underlying(underlying);
    const std::string maturity = argv[2];
    c->set_maturity(maturity);
    c->set_delta(atof(argv[3]));
    c->set_vega(atof(argv[4]));
    c->set_skew(atof(argv[5]));
    c->set_convex(atof(argv[6]));
    c->set_cash(atof(argv[7]));
    c->set_price(atof(argv[8]));
    c->set_multiplier(atof(argv[9]));
    // LOG_INF << strategy << " " << underlying << " " << maturity << " " << sizeof(*caches) / sizeof(CreditMap);
    (*caches)[strategy][underlying][maturity] = c;
  } else {
    LOG_ERR << "Failed to find underlying " << underlying;
  }
  return 0;
}

int CreditDB::RecordCallback(void *data, int argc, char **argv, char **col_name) {
  auto *tmp = static_cast<std::tuple<std::vector<CreditMap>*, InstrumentDB*>*>(data);
  auto *caches = std::get<0>(*tmp);
  auto *instrument_db = std::get<1>(*tmp);

  const std::string instrument = argv[1];
  auto option = instrument_db->FindOption(instrument);
  if (option) {
    auto strategy = static_cast<Proto::StrategyType>(atoi(argv[0]));
    auto &cache = (*caches)[strategy];
    auto it = cache.find(option->hedge_underlying());
    if (it != cache.end()) {
      auto itr = it->second.find(option->maturity());
      if (itr != it->second.end()) {
        auto *record = itr->second->add_records();
        record->set_option(instrument);
        record->set_credit(atof(argv[2]));
      }
    }
  }
  return 0;
}
