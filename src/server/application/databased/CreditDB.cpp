#include "CreditDB.h"
#include "base/logger/Logging.h"
#include "model/Message.h"
#include "boost/format.hpp"

CreditDB::CreditDB(ConcurrentSqliteDB &db, const std::string &table_name,
    InstrumentDB &instrument_db, ExchangeParameterDB &exchange_db)
  : DbBase(db, table_name), instrument_db_(instrument_db), trading_day_(exchange_db.TradingDay())
{}

void CreditDB::RefreshCache()
{
  char sql[1024];
  sprintf(sql, "DELETE FROM %s WHERE maturity<'%s'", table_name_.c_str(), trading_day_.c_str());
  ExecSql(sql);

  sprintf(sql, "SELECT * FROM %s", table_name_.c_str());
  auto data = std::make_tuple(&caches_, &instrument_db_);
  ExecSql(sql, &data, &CreditDB::Callback);
}

void CreditDB::RegisterCallback(
    base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher)
{
  dispatcher.RegisterCallback<Proto::CreditReq>(
    std::bind(&CreditDB::OnRequest, this, std::placeholders::_1));
}

base::ProtoMessagePtr CreditDB::OnRequest(const std::shared_ptr<Proto::CreditReq> &msg)
{
  LOG_INF << "Credit request: " << msg->ShortDebugString();
  auto reply = Message::NewProto<Proto::CreditRep>();
  Proto::RequestType type = msg->type();
  if (type == Proto::RequestType::Get)
  {
    for (auto &cache: caches_)
    {
      for (auto &credit : cache)
      {
        for (auto &c : credit.second)
        {
          auto *tmp = reply->add_credits();
          tmp->CopyFrom(*c.second);
        }
      }
    }
    LOG_INF << boost::format("Get %1% credits totally.") % reply->credits_size();
  }
  else if (type == Proto::RequestType::Set)
  {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &c : msg->credits())
    {
      const std::string &m = c.maturity();
      sprintf(sql,"INSERT OR REPLACE INTO %s VALUES('%s', '%d', %s, %s, %f, %f, %f, %f, %f, %f, %f)",
          table_name_.c_str(), static_cast<int>(c.strategy()), c.underlying().c_str(), m.c_str(),
          c.delta(), c.vega(), c.skew(), c.convex(), c.cash(), c.price(), c.multiplier());
      ExecSql(sql);
      auto &credits = caches_[c.strategy()][c.underlying()];
      auto it = credits.find(m);
      if (it == credits.end())
      {
        it = credits.emplace(m, Message::NewProto<Proto::Credit>()).first;
      }
      it->second->CopyFrom(c);
    }
  }
  else if (type == Proto::RequestType::Del) { }
  reply->mutable_result()->set_result(true);
  return reply;
}

int CreditDB::Callback(void *data, int argc, char **argv, char **col_name)
{
  assert(argc == 10);
  auto *tmp = static_cast<std::tuple<CreditMap**, InstrumentDB*>*>(data);
  auto *caches = std::get<0>(*tmp);
  auto *instrument_db = std::get<1>(*tmp);

  auto strategy = static_cast<Proto::StrategyType>(atoi(argv[0]));
  auto &cache = (*caches)[strategy];

  const std::string underlying = argv[1];
  auto inst = instrument_db->FindUnderlying(underlying);
  if (inst)
  {
    auto c = Message::NewProto<Proto::Credit>();
    c->set_strategy(strategy);
    c->set_underlying(underlying);
    c->set_maturity(argv[2]);
    c->set_delta(atof(argv[3]));
    c->set_vega(atof(argv[4]));
    c->set_skew(atof(argv[5]));
    c->set_convex(atof(argv[6]));
    c->set_cash(atof(argv[7]));
    c->set_price(atof(argv[8]));
    c->set_multiplier(atof(argv[9]));
    cache[underlying][argv[2]] = c;
  }
  else
  {
    LOG_ERR << "Failed to find underlying " << underlying;
  }
  return 0;
}
