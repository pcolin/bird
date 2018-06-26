#ifndef DATABASED_CREDIT_DB_H
#define DATABASED_CREDIT_DB_H

#include "DbBase.h"
#include "InstrumentDB.h"
#include "Credit.pb.h"

#include <unordered_map>

class CreditDB : public DbBase
{
public:
  CreditDB(ConcurrentSqliteDB &db, const std::string &table_name, InstrumentDB &instrument_db,
      ExchangeParameterDB &exchange_db);

private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  base::ProtoMessagePtr OnRequest(const std::shared_ptr<Proto::CreditReq> &msg);

  static int Callback(void *data, int argc, char **argv, char **col_name);

  typedef std::unordered_map<std::string,
          std::map<std::string, std::shared_ptr<Proto::Credit>>> CreditMap;
  CreditMap caches_[Proto::StrategyType::Dimer + 1];
  InstrumentDB &instrument_db_;
  std::string trading_day_;
};

#endif
