#ifndef DATABASED_SSRATE_DB_H
#define DATABASED_SSRATE_DB_H

#include <unordered_map>
#include "DbBase.h"
#include "InstrumentDB.h"
#include "ProductParameterDB.h"
#include "SSRate.pb.h"

class SSRateDB : public DbBase {
 public:
  SSRateDB(ConcurrentSqliteDB &db,
           const std::string &table_name,
           InstrumentDB &instrument_db,
           ProductParameterDB &product_db);

 private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  base::ProtoMessagePtr OnRequest(const std::shared_ptr<Proto::SSRateReq> &msg);

  static int Callback(void *data, int argc, char **argv, char **col_name);

  typedef std::unordered_map<std::string, std::map<std::string, double>> SSRateMap;
  SSRateMap cache_;
  InstrumentDB &instrument_db_;
  std::string trading_day_;
};

#endif // DATABASED_SSRATE_DB_H
