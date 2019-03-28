#ifndef DATABASED_HEDGER_DB_H
#define DATABASED_HEDGER_DB_H

#include <unordered_map>
#include "DbBase.h"
#include "InstrumentDB.h"
#include "Hedger.pb.h"

class HedgerDB : public DbBase {
 public:
  typedef std::unordered_map<std::string, std::shared_ptr<Proto::HedgerSpec>> HedgerMap;
  HedgerDB(ConcurrentSqliteDB &db,
      const std::string &table_name,
      InstrumentDB &instrument_db);

 private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(
      base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  base::ProtoMessagePtr OnRequest(const std::shared_ptr<Proto::HedgerReq> &msg);

  static int Callback(void *data, int argc, char **argv, char **col_name);

  InstrumentDB &instrument_db_;

  HedgerMap hedgers_;
};

#endif // DATABASED_HEDGER_DB_H
