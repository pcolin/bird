#ifndef DATABASED_EXCHANGE_PARAMETER_DB_H
#define DATABASED_EXCHANGE_PARAMETER_DB_H

#include "DbBase.h"
// #include "Instrument.pb.h"

#include <unordered_map>

class ExchangeParameterDB : public DbBase
{
public:
  ExchangeParameterDB(ConcurrentSqliteDB &db, const std::string &table_name);

private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  base::ProtoMessagePtr OnRequest(const std::shared_ptr<Proto::InstrumentReq> &msg);

  static int ParameterCallback(void *data, int argc, char **argv, char **col_name);
  static int HolidayCallback(void *data, int argc, char **argv, char **col_name);

  std::string table_name_;
};

#endif
