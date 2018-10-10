#ifndef DATABASED_INTEREST_RATE_DB_H
#define DATABASED_INTEREST_RATE_DB_H

#include <unordered_map>
#include "DbBase.h"
#include "InterestRate.pb.h"

class InterestRateDB : public DbBase {
 public:
  typedef std::unordered_map<int32_t, double> InterestRateMap;
  InterestRateDB(ConcurrentSqliteDB &db, const std::string &table_name);

 private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  base::ProtoMessagePtr OnRequest(const std::shared_ptr<Proto::InterestRateReq> &msg);

  static int Callback(void *data, int argc, char **argv, char **col_name);

  InterestRateMap cache_;
};

#endif // DATABASED_INTEREST_RATE_DB_H
