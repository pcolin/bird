#include "PricingSpecDB.h"
#include "base/logger/Logging.h"
#include "model/Message.h"
#include <boost/format.hpp>

PricingSpecDB::PricingSpecDB(ConcurrentSqliteDB &db, const std::string &table_name,
    const std::string &record_table_name, const InstrumentDB &instrument_db)
  : DbBase(db, table_name), record_table_name_(record_table_name), instrument_db_(instrument_db)
{}

void PricingSpecDB::RefreshCache()
{
  char sql[1024];
  sprintf(sql, "DELETE FROM %s WHERE NOT EXISTS(SELECT id FROM %s WHERE id = %s.underlying)",
      table_name_.c_str(), instrument_db_.TableName().c_str(), table_name_.c_str());
  ExecSql(sql);
  sprintf(sql, "DELETE FROM %s WHERE NOT EXISTS(SELECT id FROM %s WHERE id = %s.instrument)",
      record_table_name_.c_str(), instrument_db_.TableName().c_str(), record_table_name_.c_str());
  ExecSql(sql);

  sprintf(sql, "SELECT * FROM %s", table_name_.c_str());
  auto data = std::make_tuple(&pricings_, &instrument_db_);
  ExecSql(sql, &data, &PricingSpecDB::Callback);

  sprintf(sql, "SELECT * FROM %s", record_table_name_.c_str());
  ExecSql(sql, &data, &PricingSpecDB::RecordCallback);
}

void PricingSpecDB::RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher)
{
  dispatcher.RegisterCallback<Proto::PricingSpecReq>(
    std::bind(&PricingSpecDB::OnRequest, this, std::placeholders::_1));
}

base::ProtoMessagePtr PricingSpecDB::OnRequest(const std::shared_ptr<Proto::PricingSpecReq> &msg)
{
  LOG_INF << "PricingSpec request: " << msg->ShortDebugString();
  auto reply = Message::NewProto<Proto::PricingSpecRep>();
  if (msg->type() == Proto::RequestType::Get)
  {
    if (msg->name().empty())
    {
      for (auto it : pricings_)
      {
        reply->add_pricings()->CopyFrom(*it.second);
      }
    }
    else
    {
      auto it = pricings_.find(msg->name());
      if (it != pricings_.end())
      {
        reply->add_pricings()->CopyFrom(*it->second);
      }
      else
      {
        reply->mutable_result()->set_result(false);
        reply->mutable_result()->set_error(msg->name() + " doesn't exist");
        return reply;
      }
    }
  }
  else if (msg->type() == Proto::RequestType::Set)
  {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &pricing : msg->pricings())
    {
      const std::string &name = pricing.name();
      sprintf(sql, "INSERT OR REPLACE INTO %s VALUES('%s', '%s', %d, %d, %d, %d, %d, %f, %f)",
          table_name_.c_str(), name.c_str(), pricing.underlying().c_str(),
          static_cast<int32_t>(pricing.model()), pricing.depth(), pricing.interval(),
          static_cast<int32_t>(pricing.theo_type()), pricing.warn_tick_change(),
          pricing.elastic(), pricing.elastic_limit());
      ExecSql(sql);
      pricings_[name]->CopyFrom(pricing);

      sprintf(sql, "DELETE FROM %s WHERE name='%s'", record_table_name_.c_str(), name.c_str());
      ExecSql(sql);
      for (auto &inst : pricing.instruments())
      {
        sprintf(sql, "INSERT INTO %s VALUES('%s', '%s')", record_table_name_.c_str(), inst.c_str());
        ExecSql(sql);
      }
    }
  }
  else if (msg->type() == Proto::RequestType::Del)
  {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &pricing : msg->pricings())
    {
      const std::string &name = pricing.name();
      if (pricings_.erase(name) == 1)
      {
        sprintf(sql, "DELETE FROM %s WHERE name='%s'", table_name_.c_str(), name.c_str());
        ExecSql(sql);
        sprintf(sql, "DELETE FROM %s WHERE name='%s'", record_table_name_.c_str(), name.c_str());
        ExecSql(sql);
      }
    }
  }
  reply->mutable_result()->set_result(true);
  LOG_INF << "PricingSpec reply: " << reply->ShortDebugString();
  return reply;
}

int PricingSpecDB::Callback(void *data, int argc, char **argv, char **col_name)
{
  auto *tmp = static_cast<std::tuple<PricingSpecMap*, InstrumentDB*>*>(data);
  auto *pricings = std::get<0>(*tmp);
  auto *instrument_db = std::get<1>(*tmp);

  const std::string underlying = argv[1];
  auto inst = instrument_db->FindUnderlying(underlying);
  if (inst)
  {
    auto pricing = Message::NewProto<Proto::PricingSpec>();
    const std::string name = argv[0];
    pricing->set_name(name);
    pricing->set_underlying(underlying);
    pricing->set_model(static_cast<Proto::PricingModel>(atoi(argv[2])));
    pricing->set_depth(atoi(argv[3]));
    pricing->set_interval(atoi(argv[4]));
    pricing->set_theo_type(static_cast<Proto::UnderlyingTheoType>(atoi(argv[5])));
    pricing->set_warn_tick_change(atoi(argv[6]));
    pricing->set_elastic(atof(argv[7]));
    pricing->set_elastic_limit(atof(argv[8]));
    (*pricings)[name] = pricing;
  }
  else
  {
    LOG_ERR << underlying << " doesn't exist";
  }
  return 0;
}

int PricingSpecDB::RecordCallback(void *data, int argc, char **argv, char **col_name)
{
  auto *tmp = static_cast<std::tuple<PricingSpecMap*, InstrumentDB*>*>(data);
  auto *pricings = std::get<0>(*tmp);
  auto *instrument_db = std::get<1>(*tmp);

  const std::string id = argv[1];
  auto inst = instrument_db->FindOption(id);
  if (inst)
  {
    *((*pricings)[argv[0]]->add_instruments()) = id;
  }
  return 0;
}
