#ifndef DATABASED_CASH_LIMIT_DB_H
#define DATABASED_CASH_LIMIT_DB_H

#include "DbBase.h"
#include "CashLimit.pb.h"

#include <unordered_map>

class CashLimitDB : public DbBase
{
  typedef std::unordered_map<int32_t, std::shared_ptr<Proto::CashLimit>> CashLimitMap;
public:
  CashLimitDB(ConcurrentSqliteDB &db, const std::string &table_name);

private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  base::ProtoMessagePtr OnRequest(const std::shared_ptr<Proto::CashLimitReq> &msg);

  static int Callback(void *data, int argc, char **argv, char **col_name);

  std::string table_name_;
  CashLimitMap cache_;
};

#endif
