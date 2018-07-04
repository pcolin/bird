#include "PricerDB.h"
#include "base/logger/Logging.h"
#include "model/Message.h"
#include <boost/format.hpp>

PricerDB::PricerDB(ConcurrentSqliteDB &db, const std::string &table_name,
    InstrumentDB &instrument_db)
  : DbBase(db, table_name), instrument_db_(instrument_db)
{}

void PricerDB::RefreshCache()
{
  char sql[1024];
  sprintf(sql, "DELETE FROM %s WHERE NOT EXISTS(SELECT id FROM %s WHERE id = %s.underlying)",
      table_name_.c_str(), instrument_db_.TableName().c_str(), table_name_.c_str());
  ExecSql(sql);
  // sprintf(sql, "DELETE FROM %s WHERE NOT EXISTS(SELECT id FROM %s WHERE id = %s.instrument)",
  //     record_table_name_.c_str(), instrument_db_.TableName().c_str(), record_table_name_.c_str());
  // ExecSql(sql);

  sprintf(sql, "SELECT * FROM %s", table_name_.c_str());
  auto data = std::make_tuple(&pricers_, &instrument_db_);
  ExecSql(sql, &data, &PricerDB::Callback);

  // sprintf(sql, "SELECT * FROM %s", record_table_name_.c_str());
  // ExecSql(sql, &data, &PricerDB::RecordCallback);
}

void PricerDB::RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher)
{
  dispatcher.RegisterCallback<Proto::PricerReq>(
    std::bind(&PricerDB::OnRequest, this, std::placeholders::_1));
}

base::ProtoMessagePtr PricerDB::OnRequest(const std::shared_ptr<Proto::PricerReq> &msg)
{
  LOG_INF << "Pricer request: " << msg->ShortDebugString();
  auto reply = Message::NewProto<Proto::PricerRep>();
  if (msg->type() == Proto::RequestType::Get)
  {
    if (msg->name().empty())
    {
      for (auto it : pricers_)
      {
        reply->add_pricers()->CopyFrom(*it.second);
      }
    }
    else
    {
      auto it = pricers_.find(msg->name());
      if (it != pricers_.end())
      {
        reply->add_pricers()->CopyFrom(*it->second);
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
    for (auto &pricer : msg->pricers())
    {
      const std::string &name = pricer.name();
      sprintf(sql, "INSERT OR REPLACE INTO %s VALUES('%s', '%s', %d, %d, %d, %d, %d, %f, %f)",
          table_name_.c_str(), name.c_str(), pricer.underlying().c_str(),
          static_cast<int32_t>(pricer.model()), pricer.depth(), pricer.interval(),
          static_cast<int32_t>(pricer.theo_type()), pricer.warn_tick_change(),
          pricer.elastic(), pricer.elastic_limit());
      ExecSql(sql);
      auto it = pricers_.find(name);
      if (it != pricers_.end())
      {
        it->second->CopyFrom(pricer);
        sprintf(sql, "DELETE FROM %s WHERE name='%s'", record_table_name_.c_str(), name.c_str());
        ExecSql(sql);
      }
      else
      {
        auto p = Message::NewProto<Proto::Pricer>();
        p->CopyFrom(pricer);
        pricers_.emplace(name, p);
      }

      // for (auto &op : pricer.options())
      // {
      //   sprintf(sql, "INSERT INTO %s VALUES('%s', '%s')", record_table_name_.c_str(), name.c_str(),
      //       op.c_str());
      //   ExecSql(sql);
      // }
    }
  }
  else if (msg->type() == Proto::RequestType::Del)
  {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &pricer : msg->pricers())
    {
      const std::string &name = pricer.name();
      if (pricers_.erase(name) == 1)
      {
        sprintf(sql, "DELETE FROM %s WHERE name='%s'", table_name_.c_str(), name.c_str());
        ExecSql(sql);
        // sprintf(sql, "DELETE FROM %s WHERE name='%s'", record_table_name_.c_str(), name.c_str());
        // ExecSql(sql);
      }
    }
  }
  reply->mutable_result()->set_result(true);
  LOG_INF << "Pricer reply: " << reply->ShortDebugString();
  return reply;
}

int PricerDB::Callback(void *data, int argc, char **argv, char **col_name)
{
  auto *tmp = static_cast<std::tuple<PricerMap*, InstrumentDB*>*>(data);
  auto *pricers = std::get<0>(*tmp);
  auto *instrument_db = std::get<1>(*tmp);

  const std::string underlying = argv[1];
  auto inst = instrument_db->FindUnderlying(underlying);
  if (inst)
  {
    auto pricer = Message::NewProto<Proto::Pricer>();
    const std::string name = argv[0];
    pricer->set_name(name);
    pricer->set_underlying(underlying);
    pricer->set_model(static_cast<Proto::PricingModel>(atoi(argv[2])));
    pricer->set_depth(atoi(argv[3]));
    pricer->set_interval(atoi(argv[4]));
    pricer->set_theo_type(static_cast<Proto::UnderlyingTheoType>(atoi(argv[5])));
    pricer->set_warn_tick_change(atoi(argv[6]));
    pricer->set_elastic(atof(argv[7]));
    pricer->set_elastic_limit(atof(argv[8]));
    (*pricers)[name] = pricer;
  }
  else
  {
    LOG_ERR << underlying << " doesn't exist";
  }
  return 0;
}

// int PricerDB::RecordCallback(void *data, int argc, char **argv, char **col_name)
// {
//   auto *tmp = static_cast<std::tuple<PricerMap*, InstrumentDB*>*>(data);
//   auto *pricers = std::get<0>(*tmp);
//   auto *instrument_db = std::get<1>(*tmp);

//   const std::string id = argv[1];
//   auto inst = instrument_db->FindOption(id);
//   if (inst)
//   {
//     auto it = pricers->find(argv[0]);
//     if (it != pricers->end())
//     {
//       *(it->second->add_options()) = id;
//     }
//   }
//   return 0;
// }
