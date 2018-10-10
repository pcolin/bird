#ifndef DATABASED_STRATEGY_SWITCH_DB_H
#define DATABASED_STRATEGY_SWITCH_DB_H

#include <unordered_map>
#include "DbBase.h"
#include "InstrumentDB.h"
#include "Strategy.pb.h"

class StrategySwitchDB : public DbBase
{
 public:
  typedef std::unordered_map<std::string, std::shared_ptr<Proto::StrategySwitch>> SwitchMap;
  StrategySwitchDB(ConcurrentSqliteDB &db,
                   const std::string &table_name,
                   InstrumentDB &instrument_db);

private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  base::ProtoMessagePtr OnRequest(const std::shared_ptr<Proto::StrategySwitchReq> &msg);

  static int Callback(void *data, int argc, char **argv, char **col_name);

  InstrumentDB &instrument_db_;
  std::vector<SwitchMap> switches_;
};

#endif // DATABASED_STRATEGY_SWITCH_DB_H
