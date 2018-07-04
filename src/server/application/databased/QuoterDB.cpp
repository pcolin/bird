#include "QuoterDB.h"
#include "base/logger/Logging.h"
#include "model/Message.h"
#include <boost/format.hpp>

QuoterDB::QuoterDB(ConcurrentSqliteDB &db, const std::string &table_name,
    const std::string &record_table_name, InstrumentDB &instrument_db)
  : DbBase(db, table_name), record_table_name_(record_table_name), instrument_db_(instrument_db)
{}

void QuoterDB::RefreshCache()
{
  char sql[1024];
  sprintf(sql, "DELETE FROM %s WHERE NOT EXISTS(SELECT id FROM %s WHERE id = %s.underlying)",
      table_name_.c_str(), instrument_db_.TableName().c_str(), table_name_.c_str());
  ExecSql(sql);
  sprintf(sql, "DELETE FROM %s WHERE NOT EXISTS(SELECT id FROM %s WHERE id = %s.instrument)",
      record_table_name_.c_str(), instrument_db_.TableName().c_str(), record_table_name_.c_str());
  ExecSql(sql);

  sprintf(sql, "SELECT * FROM %s", table_name_.c_str());
  auto data = std::make_tuple(&quoters_, &instrument_db_);
  ExecSql(sql, &data, &QuoterDB::Callback);

  sprintf(sql, "SELECT * FROM %s", record_table_name_.c_str());
  // auto data1 = std::make_tuple(&quoters_, &records_, &instrument_db_);
  // ExecSql(sql, &data1, &QuoterDB::RecordCallback);
  ExecSql(sql, &data, &QuoterDB::RecordCallback);
}

void QuoterDB::RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher)
{
  dispatcher.RegisterCallback<Proto::QuoterReq>(
    std::bind(&QuoterDB::OnRequest, this, std::placeholders::_1));
}

base::ProtoMessagePtr QuoterDB::OnRequest(const std::shared_ptr<Proto::QuoterReq> &msg)
{
  LOG_INF << "Quoter request: " << msg->ShortDebugString();
  auto reply = Message::NewProto<Proto::QuoterRep>();
  if (msg->type() == Proto::RequestType::Get)
  {
    for (auto it : quoters_)
    {
      reply->add_quoters()->CopyFrom(*it.second);
      // auto *quoter = reply->add_quoters();
      // quoter->CopyFrom(*it.second);
      // auto it1 = records_.find(quoter->name());
      // if (it1 != records_.end())
      // {
      //   for (auto it2 : it1->second)
      //   {
      //     quoter->add_records()->CopyFrom(*it2.second);
      //   }
      // }
    }
  }
  else if (msg->type() == Proto::RequestType::Set)
  {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &q : msg->quoters())
    {
      const std::string &name = q.name();
      sprintf(sql, "INSERT OR REPLACE INTO %s VALUES('%s', '%s', '%s', %f, %d, %d, %d, %d, %d, %d,"
          " %d, %d, %d)",
          table_name_.c_str(), name.c_str(), q.pricer().c_str(), q.underlying().c_str(),
          q.delta_limit(), q.order_limit(), q.trade_limit(), q.bid_volume(), q.ask_volume(),
          q.response_volume(), q.depth(), q.refill_times(), q.wide_spread(), q.protection());
      ExecSql(sql);
      auto it = quoters_.find(name);
      if (it != quoters_.end())
      {
        it->second->CopyFrom(q);
        sprintf(sql, "DELETE FROM %s WHERE name='%s'", record_table_name_.c_str(), name.c_str());
        ExecSql(sql);
      }
      else
      {
        auto quoter = Message::NewProto<Proto::QuoterSpec>();
        quoter->CopyFrom(q);
        quoters_.emplace(name, quoter);
      }

      for (auto &op : q.options())
      {
        sprintf(sql, "INSERT INTO %s VALUES('%s', '%s')", record_table_name_.c_str(), name.c_str(),
            op.c_str());
        ExecSql(sql);
      }
      // if (q.records().empty())
      // {
      //   sprintf(sql, "INSERT OR REPLACE INTO %s VALUES('%s', '%s', '%s', %f, %d, %d, %d, %d, %d, %d,"
      //       " %d, %d, %d)",
      //       table_name_.c_str(), name.c_str(), q.pricer().c_str(), q.underlying().c_str(),
      //       q.delta_limit(), q.order_limit(), q.trade_limit(), q.bid_volume(), q.ask_volume(),
      //       q.response_volume(), q.depth(), q.refill_times(), q.wide_spread(), q.protection());
      //   ExecSql(sql);
      //   auto it = quoters_.find(name);
      //   if (it != quoters_.end())
      //   {
      //     it->second->CopyFrom(q);
      //   }
      //   else
      //   {
      //     auto quoter = Message::NewProto<Proto::QuoterSpec>();
      //     quoter->CopyFrom(q);
      //     quoters_.emplace(name, quoter);
      //   }
      // }
      // else
      // {
      //   auto it = records_.find(name);
      //   if (it == records_.end())
      //   {
      //     it = records_.emplace(name,
      //         std::unordered_map<std::string, std::shared_ptr<Proto::QuoterRecord>>()).first;
      //   }
      //   for (auto &r : q.records())
      //   {
      //     auto it1 = it->second.find(r.instrument());
      //     if (it1 == it->second.end())
      //     {
      //       it1 = it->second.emplace(r.instrument(), Message::NewProto<Proto::QuoterRecord>()).first;
      //     }
      //     if (r.credit() > 0 || r.multiplier() > 0)
      //     {
      //       it1->second->set_credit(r.credit());
      //       it1->second->set_multiplier(r.multiplier());
      //     }
      //     else
      //     {
      //       it1->second->set_is_bid(r.is_bid());
      //       it1->second->set_is_ask(r.is_ask());
      //       it1->second->set_is_qr(r.is_qr());
      //     }
      //     sprintf(sql, "INSERT OR REPLACE INTO %s VALUES('%s', '%s', %f, %f, %d, %d, %d)",
      //         record_table_name_.c_str(), name.c_str(), it1->first.c_str(),
      //         it1->second->credit(), it1->second->multiplier(),
      //         it1->second->is_bid(), it1->second->is_ask(), it1->second->is_qr());
      //     ExecSql(sql);
      //   }
      // }
    }

    // for (auto &op : quoter.options())
    // {
    //   sprintf(sql, "INSERT INTO %s VALUES('%s', '%s')", record_table_name_.c_str(), name.c_str(),
    //       op.c_str());
    //   ExecSql(sql);
    // }
  }
  else if (msg->type() == Proto::RequestType::Del)
  {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &q : msg->quoters())
    {
      const std::string &name = q.name();
      if (quoters_.erase(name) == 1)
      {
        sprintf(sql, "DELETE FROM %s WHERE name='%s'", table_name_.c_str(), name.c_str());
        ExecSql(sql);
        sprintf(sql, "DELETE FROM %s WHERE name='%s'", record_table_name_.c_str(), name.c_str());
        ExecSql(sql);
      }
      // if (records_.erase(name) == 1)
      // {
      //   sprintf(sql, "DELETE FROM %s WHERE name='%s'", record_table_name_.c_str(), name.c_str());
      //   ExecSql(sql);
      // }
    }
  }
  reply->mutable_result()->set_result(true);
  LOG_INF << "Quoter reply: " << reply->ShortDebugString();
  return reply;
}

int QuoterDB::Callback(void *data, int argc, char **argv, char **col_name)
{
  auto *tmp = static_cast<std::tuple<QuoterMap*, InstrumentDB*>*>(data);
  auto *quoters = std::get<0>(*tmp);
  auto *instrument_db = std::get<1>(*tmp);

  const std::string underlying = argv[2];
  auto inst = instrument_db->FindUnderlying(underlying);
  if (inst)
  {
    auto quoter = Message::NewProto<Proto::QuoterSpec>();
    const std::string name = argv[0];
    quoter->set_name(name);
    quoter->set_pricer(argv[1]);
    quoter->set_underlying(underlying);
    quoter->set_delta_limit(atof(argv[3]));
    quoter->set_order_limit(atoi(argv[4]));
    quoter->set_trade_limit(atoi(argv[5]));
    quoter->set_bid_volume(atoi(argv[6]));
    quoter->set_ask_volume(atoi(argv[7]));
    quoter->set_response_volume(atoi(argv[8]));
    quoter->set_depth(atoi(argv[9]));
    quoter->set_refill_times(atoi(argv[10]));
    quoter->set_wide_spread(atoi(argv[11]) == 1);
    quoter->set_protection(atoi(argv[12]) == 1);
    (*quoters)[name] = quoter;
  }
  else
  {
    LOG_ERR << underlying << " doesn't exist";
  }
  return 0;
}

int QuoterDB::RecordCallback(void *data, int argc, char **argv, char **col_name)
{
  auto *tmp = static_cast<std::tuple<QuoterMap*, InstrumentDB*>*>(data);
  auto *quoters = std::get<0>(*tmp);
  auto *instrument_db = std::get<1>(*tmp);

  const std::string instrument = argv[1];
  auto option = instrument_db->FindOption(instrument);
  if (option)
  {
    // const std::string name = argv[0];
    auto it = quoters->find(argv[0]);
    if (it != quoters->end())
    {
      it->second->add_options(std::move(instrument));
      // auto record = Message::NewProto<Proto::QuoterRecord>();
      // record->set_instrument(instrument);
      // record->set_credit(atof(argv[2]));
      // record->set_multiplier(atof(argv[3]));
      // record->set_is_bid(atoi(argv[4]) == 1);
      // record->set_is_ask(atoi(argv[5]) == 1);
      // record->set_is_qr(atoi(argv[6]) == 1);
      // (*records)[name][instrument] = record;
    }
  }
  return 0;
}
