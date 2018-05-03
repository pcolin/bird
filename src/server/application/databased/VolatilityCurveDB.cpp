#include "VolatilityCurveDB.h"
#include "base/logger/Logging.h"
#include "model/Message.h"

VolatilityCurveDB::VolatilityCurveDB(ConcurrentSqliteDB &db, const std::string &table_name,
    InstrumentDB &instrument_db, ExchangeParameterDB &exchange_db)
  : DbBase(db, table_name), instrument_db_(instrument_db), trading_day_(exchange_db.TradingDay())
{}

void VolatilityCurveDB::RefreshCache()
{
  char sql[1024];
  sprintf(sql, "DELETE FROM %s WHERE maturity<'%s'", table_name_.c_str(), trading_day_.c_str());
  ExecSql(sql);

  sprintf(sql, "SELECT * FROM %s", table_name_.c_str());
  auto data = std::make_tuple(&cache_, &instrument_db_);
  ExecSql(sql, &data, &VolatilityCurveDB::Callback);
}

void VolatilityCurveDB::RegisterCallback(
    base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher)
{
  dispatcher.RegisterCallback<Proto::VolatilityCurveReq>(
    std::bind(&VolatilityCurveDB::OnRequest, this, std::placeholders::_1));
}

base::ProtoMessagePtr VolatilityCurveDB::OnRequest(
    const std::shared_ptr<Proto::VolatilityCurveReq> &msg)
{
  LOG_INF << "Volatility curve request: " << msg->ShortDebugString();
  auto reply = Message::NewProto<Proto::VolatilityCurveRep>();
  Proto::RequestType type = msg->type();
  if (type == Proto::RequestType::Get)
  {
    if (msg->underlying().empty())
    {
      for (auto &vc: cache_)
      {
        for (auto &c : vc.second)
        {
          auto *tmp = reply->add_curves();
          tmp->CopyFrom(*c.second);
        }
      }
    }
    else
    {
      auto it = cache_.find(msg->underlying());
      if (it != cache_.end())
      {
        for (auto &c : it->second)
        {
          auto *vc = reply->add_curves();
          vc->CopyFrom(*c.second);
        }
      }
      else
      {
        reply->mutable_result()->set_result(false);
        reply->mutable_result()->set_error(msg->underlying() + " doesn't exist");
        return reply;
      }
    }
    LOG_INF << boost::format("Get %1% volatility curves totally.") % reply->curves_size();
  }
  else if (type == Proto::RequestType::Set)
  {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &c : msg->curves())
    {
      const std::string &m = c.maturity();
      sprintf(sql, "INSERT OR REPLACE INTO %s VALUES('%s', '%s', %f, %f, %f, %f, %f, %f, %f, %f, %f,"
          " %f, %f, %f, %f, %f)", table_name_.c_str(), c.underlying().c_str(), m.c_str(), c.spot(),
          c.atm_vol(), c.skew(), c.call_convex(), c.put_convex(), c.call_slope(), c.put_slope(),
          c.call_cutoff(), c.put_cutoff(), c.vcr(), c.scr(), c.ccr(), c.spcr(), c.sccr());
      ExecSql(sql);
      auto &vc = cache_[c.underlying()];
      auto it = vc.find(m);
      if (it == vc.end())
      {
        it = vc.emplace(m, Message::NewProto<Proto::VolatilityCurve>()).first;
      }
      it->second->CopyFrom(c);
    }
  }
  else if (type == Proto::RequestType::Del)
  {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &c : msg->curves())
    {
      sprintf(sql, "DELETE FROM %s WHERE underlying='%s'AND maturity='%s'", table_name_.c_str(),
          c.underlying().c_str(), c.maturity().c_str());
      ExecSql(sql);
      auto it = cache_.find(c.underlying());
      if (it != cache_.end())
      {
        it->second.erase(c.maturity());
      }
    }
  }
  reply->mutable_result()->set_result(true);
  return reply;
}

int VolatilityCurveDB::Callback(void *data, int argc, char **argv, char **col_name)
{
  assert(argc == 16);
  auto *tmp = static_cast<std::tuple<VolatilityCurveMap*, InstrumentDB*>*>(data);
  auto *cache = std::get<0>(*tmp);
  auto *instrument_db = std::get<1>(*tmp);

  const std::string underlying = argv[0];
  auto inst = instrument_db->FindUnderlying(underlying);
  if (inst)
  {
    auto vc = Message::NewProto<Proto::VolatilityCurve>();
    vc->set_underlying(underlying);
    vc->set_maturity(argv[1]);
    vc->set_spot(atof(argv[2]));
    vc->set_atm_vol(atof(argv[3]));
    vc->set_skew(atof(argv[4]));
    vc->set_call_convex(atof(argv[5]));
    vc->set_put_convex(atof(argv[6]));
    vc->set_call_slope(atof(argv[7]));
    vc->set_put_slope(atof(argv[8]));
    vc->set_call_cutoff(atof(argv[9]));
    vc->set_put_cutoff(atof(argv[10]));
    vc->set_vcr(atof(argv[11]));
    vc->set_scr(atof(argv[12]));
    vc->set_ccr(atof(argv[13]));
    vc->set_spcr(atof(argv[14]));
    vc->set_sccr(atof(argv[15]));
    (*cache)[underlying][argv[1]] = vc;
  }
  else
  {
    LOG_ERR << "Failed to find underlying " << underlying;
  }
  return 0;
}
