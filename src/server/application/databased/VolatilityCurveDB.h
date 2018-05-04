#ifndef DATABASED_VOLATILITY_CURVE_DB_H
#define DATABASED_VOLATILITY_CURVE_DB_H

#include "DbBase.h"
#include "InstrumentDB.h"
#include "ExchangeParameterDB.h"
#include "Volatility.pb.h"

#include <unordered_map>

class VolatilityCurveDB : public DbBase
{
public:
  VolatilityCurveDB(ConcurrentSqliteDB &db, const std::string &table_name,
      InstrumentDB &instrument_db, ExchangeParameterDB &exchange_db);

private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  base::ProtoMessagePtr OnRequest(const std::shared_ptr<Proto::VolatilityCurveReq> &msg);

  static int Callback(void *data, int argc, char **argv, char **col_name);

  typedef std::unordered_map<std::string,
          std::map<std::string, std::shared_ptr<Proto::VolatilityCurve>>> VolatilityCurveMap;
  VolatilityCurveMap cache_;
  InstrumentDB &instrument_db_;
  std::string trading_day_;
};

#endif