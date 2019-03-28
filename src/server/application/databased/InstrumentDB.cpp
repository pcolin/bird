#include "InstrumentDB.h"
#include <sstream>
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"
#include "model/Message.h"
#include "boost/format.hpp"

InstrumentDB::InstrumentDB(ConcurrentSqliteDB &db,
                           const std::string &table_name,
                           ProductParameterDB &product_db)
    : DbBase(db, table_name), trading_date_(product_db.TradingDay()) {}

std::shared_ptr<Proto::Instrument> InstrumentDB::FindOption(const std::string &id) {
  auto it = options_.find(id);
  return it != options_.end() ? it->second : nullptr;
}

std::shared_ptr<Proto::Instrument> InstrumentDB::FindUnderlying(const std::string &id) {
  auto it = underlyings_.find(id);
  return it != underlyings_.end() ? it->second : nullptr;
}

std::shared_ptr<Proto::Instrument> InstrumentDB::FindInstrument(const std::string &id) {
  auto inst = FindOption(id);
  return inst ? inst : FindUnderlying(id);
}

void InstrumentDB::RefreshCache() {
  char query[1024];
  if (EnvConfig::GetInstance()->GetBool(EnvVar::DEL_EXPIRED_INSTRUMENT, true)) {
    sprintf(query, "DELETE FROM %s WHERE maturity<'%s'", table_name_.c_str(), trading_date_.c_str());
    ExecSql(query);
  }
  sprintf(query, "SELECT * FROM %s", table_name_.c_str());
  ExecSql(query, &underlyings_, &InstrumentDB::UnderlyingCallback);
  ExecSql(query, this, &InstrumentDB::OptionCallback);
}

void InstrumentDB::RegisterCallback(
    base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher) {
  dispatcher.RegisterCallback<Proto::InstrumentReq>(
      std::bind(&InstrumentDB::OnRequest, this, std::placeholders::_1));
}

base::ProtoMessagePtr InstrumentDB::OnRequest(const std::shared_ptr<Proto::InstrumentReq> &msg) {
  LOG_INF << "instrument request: " << msg->ShortDebugString();
  auto reply = std::make_shared<Proto::InstrumentRep>();
  if (msg->type() == Proto::RequestType::Get) {
    for (auto &underlying : underlyings_) {
      if (msg->exchange() == underlying.second->exchange()) {
        reply->add_instruments()->CopyFrom(*underlying.second);
      }
    }
    for (auto &option : options_) {
      if (msg->exchange() == option.second->exchange()) {
        reply->add_instruments()->CopyFrom(*option.second);
      }
    }
    LOG_INF << boost::format("get %1% instruments totally.") % reply->instruments_size();
  } else if (msg->type() == Proto::RequestType::Set) {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &inst : msg->instruments()) {
      if (!inst.symbol().empty()) {
        sprintf(sql, "INSERT OR REPLACE INTO %s VALUES('%s', '%s', '%s', %d, %d, %d, '%s', '%s', %d, %d, "
                     "%f, %f, %f, %f, %d, %f, %f, %f, %d, '%s', %d, %d, %f)",
                table_name_.c_str(), inst.id().c_str(), inst.symbol().c_str(), inst.product().c_str(),
                static_cast<int32_t>(inst.exchange()), static_cast<int32_t>(inst.type()),
                static_cast<int32_t>(inst.currency()), inst.underlying().c_str(),
                inst.hedge_underlying().c_str(), static_cast<int32_t>(inst.status()), inst.lot(),
                inst.tick(), inst.multiplier(), inst.highest(), inst.lowest(), inst.commission(),
                inst.open_commission(), inst.close_commission(), inst.close_today_commission(),
                static_cast<int32_t>(inst.call_put()), inst.maturity().c_str(),
                static_cast<int32_t>(inst.exercise()), static_cast<int32_t>(inst.settlement()),
                inst.strike());
        ExecSql(sql);
        UpdateInstrument(inst,
                         inst.type() == Proto::InstrumentType::Option ? options_ : underlyings_);
      } else if (inst.status() != Proto::InstrumentStatus::Unknown) {
        sprintf(sql, "UPDATE %s SET status = %d WHERE id = '%s'", table_name_.c_str(),
                static_cast<int32_t>(inst.status()), inst.id().c_str());
        ExecSql(sql);
        UpdateInstrumentStatus(inst,
            inst.type() == Proto::InstrumentType::Option ? options_ : underlyings_);
      }
    }
  } else if (msg->type() == Proto::RequestType::Del) {}
  reply->mutable_result()->set_result(true);
  return reply;
}

void InstrumentDB::UpdateInstrument(const Proto::Instrument &inst, InstrumentMap &cache) {
  auto it = cache.find(inst.id());
  if (it != cache.end()) {
    it->second->CopyFrom(inst);
  } else {
    auto instrument = std::make_shared<Proto::Instrument>();
    instrument->CopyFrom(inst);
    cache.emplace(inst.id(), instrument);
  }
}

void InstrumentDB::UpdateInstrumentStatus(const Proto::Instrument &inst, InstrumentMap &cache) {
  auto it = cache.find(inst.id());
  if (it != cache.end()) {
    it->second->set_status(inst.status());
  }
}

int InstrumentDB::UnderlyingCallback(void *data, int argc, char **argv, char **col_name) {
  auto type = static_cast<Proto::InstrumentType>(atoi(argv[4]));
  if (type != Proto::InstrumentType::Option) {
    auto &cache = *static_cast<InstrumentMap*>(data);
    std::string id = argv[0];
    auto inst = std::make_shared<Proto::Instrument>();
    inst->set_id(id);
    inst->set_symbol(argv[1]);
    inst->set_product(argv[2]);
    inst->set_exchange(static_cast<Proto::Exchange>(atoi(argv[3])));
    inst->set_type(type);
    inst->set_currency(static_cast<Proto::Currency>(atoi(argv[5])));
    inst->set_underlying(argv[6]);
    inst->set_hedge_underlying(argv[7]);
    inst->set_status(static_cast<Proto::InstrumentStatus>(atoi(argv[8])));
    inst->set_lot(atoi(argv[9]));
    inst->set_tick(atof(argv[10]));
    inst->set_multiplier(atof(argv[11]));
    inst->set_highest(atof(argv[12]));
    inst->set_lowest(atof(argv[13]));
    inst->set_commission(static_cast<Proto::CommissionType>(atoi(argv[14])));
    inst->set_open_commission(atof(argv[15]));
    inst->set_close_commission(atof(argv[16]));
    inst->set_close_today_commission(atof(argv[17]));
    inst->set_maturity(argv[19]);
    cache[id] = inst;
  }
  return 0;
}

int InstrumentDB::OptionCallback(void *data, int argc, char **argv, char **col_name) {
  auto type = static_cast<Proto::InstrumentType>(atoi(argv[4]));
  if (type == Proto::InstrumentType::Option) {
    auto *db = static_cast<InstrumentDB*>(data);
    auto &cache = db->options_;
    std::string id = argv[0];
    auto inst = std::make_shared<Proto::Instrument>();
    inst->set_id(id);
    inst->set_symbol(argv[1]);
    inst->set_product(argv[2]);
    inst->set_exchange(static_cast<Proto::Exchange>(atoi(argv[3])));
    inst->set_type(type);
    inst->set_currency(static_cast<Proto::Currency>(atoi(argv[5])));
    inst->set_underlying(argv[6]);
    inst->set_hedge_underlying(argv[7]);
    inst->set_status(static_cast<Proto::InstrumentStatus>(atoi(argv[8])));
    inst->set_lot(atoi(argv[9]));
    inst->set_tick(atof(argv[10]));
    inst->set_multiplier(atof(argv[11]));
    inst->set_highest(atof(argv[12]));
    inst->set_lowest(atof(argv[13]));
    inst->set_commission(static_cast<Proto::CommissionType>(atoi(argv[14])));
    inst->set_open_commission(atof(argv[15]));
    inst->set_close_commission(atof(argv[16]));
    inst->set_close_today_commission(atof(argv[17]));
    inst->set_call_put(static_cast<Proto::OptionType>(atoi(argv[18])));
    inst->set_maturity(argv[19]);
    inst->set_exercise(static_cast<Proto::ExerciseType>(atoi(argv[20])));
    inst->set_settlement(static_cast<Proto::SettlementType>(atoi(argv[21])));
    inst->set_strike(atof(argv[22]));
    cache[id] = inst;
  }
  return 0;
}
