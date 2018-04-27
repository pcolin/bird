#include "SSRateDB.h"
#include "base/logger/Logging.h"
#include "model/Message.h"

SSRateDB::SSRateDB(ConcurrentSqliteDB &db, const std::string &table_name,
    InstrumentDB &instrument_db, ExchangeParameterDB &exchange_db)
  : DbBase(db, table_name), instrument_db_(instrument_db), trading_day_(exchange_db.TradingDay())
{}

void SSRateDB::RefreshCache()
{
  char sql[1024];
  sprintf(sql, "DELETE FROM %s WHERE date<'%s'", table_name_.c_str(), trading_day_.c_str());
  ExecSql(sql);

  sprintf(sql, "SELECT * FROM %s", table_name_.c_str());
  auto data = std::make_tuple(&cache_, &instrument_db_);
  ExecSql(sql, &data, &SSRateDB::Callback);
}

void SSRateDB::RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher)
{
  dispatcher.RegisterCallback<Proto::SSRateReq>(
    std::bind(&SSRateDB::OnRequest, this, std::placeholders::_1));
}

base::ProtoMessagePtr SSRateDB::OnRequest(const std::shared_ptr<Proto::SSRateReq> &msg)
{
  LOG_INF << "SSRate request: " << msg->ShortDebugString();
  auto reply = Message::NewProto<Proto::SSRateRep>();
  Proto::RequestType type = msg->type();
  if (type == Proto::RequestType::Get)
  {
    auto it = cache_.find(msg->underlying());
    if (it != cache_.end())
    {
      for (auto &r : it->second)
      {
        auto *ssr = reply->add_rates();
        ssr->set_underlying(msg->underlying());
        ssr->set_date(r.first);
        ssr->set_rate(r.second);
      }
    }
    else
    {
      reply->mutable_result()->set_result(false);
      reply->mutable_result()->set_error(msg->underlying() + " doesn't exist");
      return reply;
    }
  }
  else if (type == Proto::RequestType::Set)
  {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &r : msg->rates())
    {
      /// to be done...
    }
  }
  else if (type == Proto::RequestType::Del)
  {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &d : msg->rates())
    {
      /// to be done...
    }
  }
  reply->mutable_result()->set_result(true);
  return reply;
}

int SSRateDB::Callback(void *data, int argc, char **argv, char **col_name)
{
  assert(argc == 3);
  auto *tmp = static_cast<std::tuple<SSRateMap*, InstrumentDB*>*>(data);
  auto *cache = std::get<0>(*tmp);
  auto *instrument_db = std::get<1>(*tmp);

  const std::string id = argv[0];
  auto inst = instrument_db->FindOption(id);
  if (inst)
  {
    (*cache)[id][argv[1]] = atof(argv[2]);
  }
}
