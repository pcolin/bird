#include "InstrumentDB.h"
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"
#include "model/Message.h"

// #include <chrono>
// #include <ctime>
// #include <iomanip>
#include <sstream>
#include <boost/format.hpp>

InstrumentDB::InstrumentDB(ConcurrentSqliteDB &db, const std::string &table_name,
    ExchangeParameterDB &exchange_db)
  : DbBase(db, table_name), trading_date_(exchange_db.TradingDay())
{
  // std::ostringstream oss;
  // auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  // oss << std::put_time(std::localtime(&t), "%Y%m%d");
  // trading_date_ = oss.str();
  // LOG_INF << "Trading date is " << trading_date_;
}

std::shared_ptr<Proto::Instrument> InstrumentDB::FindOption(const std::string &id)
{
  auto it = options_.find(id);
  return it != options_.end() ? it->second : nullptr;
}

std::shared_ptr<Proto::Instrument> InstrumentDB::FindUnderlying(const std::string &id)
{
  auto it = underlyings_.find(id);
  return it != underlyings_.end() ? it->second : nullptr;
}

void InstrumentDB::RefreshCache()
{
  char query[1024];
  if (EnvConfig::GetInstance()->GetBool(EnvVar::DEL_EXPIRE_INST, true))
  {
    sprintf(query, "DELETE FROM %s WHERE maturity<'%s'", table_name_.c_str(), trading_date_.c_str());
    ExecSql(query);
  }
  sprintf(query, "SELECT * FROM %s", table_name_.c_str());
  ExecSql(query, &underlyings_, &InstrumentDB::UnderlyingCallback);
  ExecSql(query, this, &InstrumentDB::OptionCallback);
}

void InstrumentDB::RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher)
{
  dispatcher.RegisterCallback<Proto::InstrumentReq>(
    std::bind(&InstrumentDB::OnRequest, this, std::placeholders::_1));
}

base::ProtoMessagePtr InstrumentDB::OnRequest(const std::shared_ptr<Proto::InstrumentReq> &msg)
{
  LOG_INF << "Instrument request: " << msg->ShortDebugString();
  auto reply = Message::NewProto<Proto::InstrumentRep>();
  if (msg->type() == Proto::RequestType::Get)
  {
    for (auto &underlying : underlyings_)
    {
      if (msg->exchange() == underlying.second->exchange())
      {
        reply->add_instruments()->CopyFrom(*underlying.second);
      }
    }
    for (auto &option : options_)
    {
      if (msg->exchange() == option.second->exchange())
      {
        reply->add_instruments()->CopyFrom(*option.second);
      }
    }
    LOG_INF << boost::format("Get %1% instruments totally.") % reply->instruments_size();
  }
  else if (msg->type() == Proto::RequestType::Set)
  {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &inst : msg->instruments())
    {
      sprintf(sql, "INSERT OR REPLACE INTO %s VALUES('%s', '%s', %d, %d, %d, '%s', '%s', %f, "
                   "%f, %f, %f, %d, '%s', %d, %d, %f)",
              table_name_.c_str(), inst.id().c_str(), inst.symbol().c_str(),
              static_cast<int32_t>(inst.exchange()), static_cast<int32_t>(inst.type()),
              static_cast<int32_t>(inst.currency()), inst.underlying().c_str(),
              inst.hedge_underlying().c_str(), inst.tick(), inst.multiplier(), inst.highest(),
              inst.lowest(), static_cast<int32_t>(inst.call_put()), inst.maturity().c_str(),
              static_cast<int32_t>(inst.exercise()), static_cast<int32_t>(inst.settlement()),
              inst.strike());
      ExecSql(sql);
      UpdateInstrument(inst, inst.type() == Proto::InstrumentType::Option ? options_ : underlyings_);
    }
  }
  else if (msg->type() == Proto::RequestType::Del)
  {}
  reply->mutable_result()->set_result(true);
  return reply;
}

void InstrumentDB::UpdateInstrument(const Proto::Instrument &inst, InstrumentMap &cache)
{
  auto it = cache.find(inst.id());
  if (it != cache.end())
  {
    it->second->CopyFrom(inst);
  }
  else
  {
    auto instrument = Message::NewProto<Proto::Instrument>();
    instrument->CopyFrom(inst);
    cache.emplace(inst.id(), instrument);
  }
}

int InstrumentDB::UnderlyingCallback(void *data, int argc, char **argv, char **col_name)
{
  auto type = static_cast<Proto::InstrumentType>(atoi(argv[3]));
  if (type != Proto::InstrumentType::Option)
  {
    auto &cache = *static_cast<InstrumentMap*>(data);
    std::string id = argv[0];
    auto inst = Message::NewProto<Proto::Instrument>();
    inst->set_id(id);
    inst->set_symbol(argv[1]);
    inst->set_exchange(static_cast<Proto::Exchange>(atoi(argv[2])));
    inst->set_type(type);
    inst->set_currency(static_cast<Proto::Currency>(atoi(argv[4])));
    inst->set_underlying(argv[5]);
    inst->set_hedge_underlying(argv[6]);
    inst->set_tick(atof(argv[7]));
    inst->set_multiplier(atof(argv[8]));
    inst->set_highest(atof(argv[9]));
    inst->set_lowest(atof(argv[10]));
    inst->set_maturity(argv[12]);
    cache[id] = inst;
  }
  return 0;
}

int InstrumentDB::OptionCallback(void *data, int argc, char **argv, char **col_name)
{
  auto type = static_cast<Proto::InstrumentType>(atoi(argv[3]));
  if (type == Proto::InstrumentType::Option)
  {
    auto *db = static_cast<InstrumentDB*>(data);
    auto &cache = db->options_;
    std::string id = argv[0];
    auto inst = Message::NewProto<Proto::Instrument>();
    inst->set_id(id);
    inst->set_symbol(argv[1]);
    inst->set_exchange(static_cast<Proto::Exchange>(atoi(argv[2])));
    inst->set_type(type);
    inst->set_currency(static_cast<Proto::Currency>(atoi(argv[4])));
    inst->set_underlying(argv[5]);
    inst->set_hedge_underlying(argv[6]);
    inst->set_tick(atof(argv[7]));
    inst->set_multiplier(atof(argv[8]));
    inst->set_highest(atof(argv[9]));
    inst->set_lowest(atof(argv[10]));
    inst->set_call_put(static_cast<Proto::OptionType>(atoi(argv[11])));
    inst->set_maturity(argv[12]);
    inst->set_exercise(static_cast<Proto::ExerciseType>(atoi(argv[13])));
    inst->set_settlement(static_cast<Proto::SettlementType>(atoi(argv[14])));
    inst->set_strike(atof(argv[15]));
    cache[id] = inst;
  }
  return 0;
}
