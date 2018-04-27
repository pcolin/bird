#ifndef DATABASED_PRICING_SPEC_DB_H
#define DATABASED_PRICING_SPEC_DB_H

#include "DbBase.h"
#include "InstrumentDB.h"
#include "PricingSpec.pb.h"

#include <unordered_map>

class PricingSpecDB : public DbBase
{
  typedef std::unordered_map<std::string, std::shared_ptr<Proto::PricingSpec>> PricingSpecMap;
public:
  PricingSpecDB(ConcurrentSqliteDB &db, const std::string &table_name,
      const std::string &record_table_name, InstrumentDB &instrument_db);

private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  base::ProtoMessagePtr OnRequest(const std::shared_ptr<Proto::PricingSpecReq> &msg);
  void UpdatePricing(const Proto::PricingSpec &pricing, PricingSpecMap &cache);


  static int Callback(void *data, int argc, char **argv, char **col_name);
  static int RecordCallback(void *data, int argc, char **argv, char **col_name);

  std::string record_table_name_;
  InstrumentDB &instrument_db_;

  PricingSpecMap pricings_;
};

#endif
