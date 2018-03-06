#include "CashLimitDB.h"
#include "base/logger/Logging.h"
#include "model/Message.h"

CashLimitDB::CashLimitDB(ConcurrentSqliteDB &db, const std::string &table_name)
  : DbBase(db), table_name_(table_name)
{}

void CashLimitDB::RefreshCache()
{
  char query[1024];
  sprintf(query, "SELECT * FROM %s", table_name_.c_str());
  ExecSql(query, &cache_, &CashLimitDB::Callback);
}

void CashLimitDB::RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher)
{
  dispatcher.RegisterCallback<Proto::CashLimitReq>(
    std::bind(&CashLimitDB::OnRequest, this, std::placeholders::_1));
}

base::ProtoMessagePtr CashLimitDB::OnRequest(const std::shared_ptr<Proto::CashLimitReq> &msg)
{
  LOG_INF << "Cash limit request: " << msg->ShortDebugString();
  auto reply = Message::NewProto<Proto::CashLimitRep>();
  if (msg->type() == Proto::RequestType::Get)
  {
    for (auto &it : cache_)
    {
      reply->add_limits()->CopyFrom(*it.second);
    }
  }
  else if (msg->type() == Proto::RequestType::Set)
  {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &limit : msg->limits())
    {
      int32_t exchange = limit.exchange();
      auto it = cache_.find(exchange);
      if (it != cache_.end())
      {
        it->second->set_limit(limit.limit());
      }
      else
      {
        auto lt = Message::NewProto<Proto::CashLimit>();
        lt->CopyFrom(limit);
        cache_.emplace(exchange, lt);
      }
      sprintf(sql, "INSERT OR REPLACE INTO %s VALUES(%d, %f)",
              table_name_.c_str(), static_cast<int32_t>(limit.exchange()), limit.limit());
      ExecSql(sql);
    }
  }
  else if (msg->type() == Proto::RequestType::Del)
  {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &limit : msg->limits())
    {
      cache_.erase(limit.exchange());
      sprintf(sql, "DELETE FROM %s where exchange = %d",
              table_name_.c_str(), static_cast<int32_t>(limit.exchange()));
      ExecSql(sql);
    }
  }
  reply->mutable_result()->set_result(true);
  return reply;
}

int CashLimitDB::Callback(void *data, int argc, char **argv, char **col_name)
{
  auto limit = Message::NewProto<Proto::CashLimit>();
  int32_t exchange = atoi(argv[0]);
  limit->set_exchange(static_cast<Proto::Exchange>(exchange));
  limit->set_limit(atof(argv[1]));
  auto &cache = *static_cast<CashLimitMap*>(data);
  cache[exchange] = limit;
  return 0;
}
