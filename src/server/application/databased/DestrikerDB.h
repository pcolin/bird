#ifndef DATABASED_DESTRIKER_DB_H
#define DATABASED_DESTRIKER_DB_H

#include "DbBase.h"
#include "InstrumentDB.h"
#include "Destriker.pb.h"

#include <unordered_map>

class DestrikerDB : public DbBase
{
public:
  DestrikerDB(ConcurrentSqliteDB &db, const std::string &table_name, InstrumentDB &instrument_db);

private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  base::ProtoMessagePtr OnRequest(const std::shared_ptr<Proto::DestrikerReq> &msg);

  static int Callback(void *data, int argc, char **argv, char **col_name);

  typedef std::unordered_map<std::string, double> DestrikerMap;
  DestrikerMap cache_;
  InstrumentDB &instrument_db_;
};

#endif
