#include "StrategySwitchDB.h"
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"
#include "model/Message.h"
#include "boost/format.hpp"

StrategySwitchDB::StrategySwitchDB(ConcurrentSqliteDB &db,
                                   const std::string &table_name,
                                   InstrumentDB &instrument_db)
    : DbBase(db, table_name),
      instrument_db_(instrument_db),
      switches_(4) {}

void StrategySwitchDB::RefreshCache() {
  char sql[1024];
  sprintf(sql, "DELETE FROM %s WHERE NOT EXISTS(SELECT id FROM %s WHERE id = %s.option)",
          table_name_.c_str(), instrument_db_.TableName().c_str(), table_name_.c_str());
  ExecSql(sql);

  sprintf(sql, "SELECT * FROM %s", table_name_.c_str());
  auto data = std::make_tuple(&switches_, &instrument_db_);
  ExecSql(sql, &data, &StrategySwitchDB::Callback);
}

void StrategySwitchDB::RegisterCallback(
    base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher) {
  dispatcher.RegisterCallback<Proto::StrategySwitchReq>(
      std::bind(&StrategySwitchDB::OnRequest, this, std::placeholders::_1));
}

base::ProtoMessagePtr StrategySwitchDB::OnRequest(
    const std::shared_ptr<Proto::StrategySwitchReq> &msg) {
  LOG_INF << "StrategySwitch request: " << msg->ShortDebugString();
  auto reply = Message::NewProto<Proto::StrategySwitchRep>();
  if (msg->type() == Proto::RequestType::Get) {
    for (auto &cache : switches_) {
      for (auto &s : cache) {
        reply->add_switches()->CopyFrom(*s.second);
      }
    }
    LOG_INF << boost::format("Get %1% strategy switches totally.") % reply->switches_size();
  } else if (msg->type() == Proto::RequestType::Set) {
    char sql[1024];
    TransactionGuard tg(this);
    for (auto &s : msg->switches()) {
      auto &cache = switches_[s.strategy()];
      const std::string &option = s.option();
      auto it = cache.find(option);
      if (it != cache.end()) {
        it->second->CopyFrom(s);
      } else {
        auto sw = Message::NewProto<Proto::StrategySwitch>();
        sw->CopyFrom(s);
        cache.emplace(option, sw);
      }
      sprintf(sql, "INSERT OR REPLACE INTO %s VALUES(%d, '%s', %d, %d, %d)", table_name_.c_str(),
              s.strategy(), option.c_str(), s.is_bid(), s.is_ask(), s.is_qr_cover());
      ExecSql(sql);
    }
  } else {}
  reply->mutable_result()->set_result(true);
  return reply;
}

int StrategySwitchDB::Callback(void *data, int argc, char **argv, char **col_name) {
  auto *tmp = static_cast<std::tuple<std::vector<SwitchMap>*, InstrumentDB*>*>(data);
  auto *caches = std::get<0>(*tmp);
  auto *instrument_db = std::get<1>(*tmp);

  const std::string option = argv[1];
  auto inst = instrument_db->FindOption(option);
  if (inst) {
    auto sw = Message::NewProto<Proto::StrategySwitch>();
    auto strategy = static_cast<Proto::StrategyType>(atoi(argv[0]));
    sw->set_strategy(strategy);
    sw->set_option(option);
    sw->set_is_bid(atoi(argv[2]) == 1);
    sw->set_is_ask(atoi(argv[3]) == 1);
    sw->set_is_qr_cover(atoi(argv[4]) == 1);
    (*caches)[strategy][option] = sw;
  }
  return 0;
}
