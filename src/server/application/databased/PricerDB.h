#ifndef DATABASED_PRICING_SPEC_DB_H
#define DATABASED_PRICING_SPEC_DB_H

#include "DbBase.h"
#include "InstrumentDB.h"
#include "Pricer.pb.h"

#include <unordered_map>

class PricerDB : public DbBase
{
  typedef std::unordered_map<std::string, std::shared_ptr<Proto::Pricer>> PricerMap;
public:
  PricerDB(ConcurrentSqliteDB &db, const std::string &table_name,
      const std::string &record_table_name, InstrumentDB &instrument_db);

private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  base::ProtoMessagePtr OnRequest(const std::shared_ptr<Proto::PricerReq> &msg);
  void UpdatePricer(const Proto::Pricer &pricer, PricerMap &cache);


  static int Callback(void *data, int argc, char **argv, char **col_name);
  static int RecordCallback(void *data, int argc, char **argv, char **col_name);

  std::string record_table_name_;
  InstrumentDB &instrument_db_;

  PricerMap pricers_;
};

#endif
