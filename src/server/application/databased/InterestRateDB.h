#ifndef DATABASED_INTEREST_RATE_DB_H
#define DATABASED_INTEREST_RATE_DB_H

#include "DbBase.h"
#include "InterestRate.pb.h"

#include <unordered_map>

class InterestRateDB : public DbBase
{
  typedef std::unordered_map<int32_t, double> InterestRateMap;
public:
  InterestRateDB(ConcurrentSqliteDB &db, const std::string &table_name);

private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  base::ProtoMessagePtr OnRequest(const std::shared_ptr<Proto::InterestRateReq> &msg);

  static int Callback(void *data, int argc, char **argv, char **col_name);

  InterestRateMap cache_;
};

#endif
