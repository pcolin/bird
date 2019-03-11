#ifndef DATABASED_HITTER_DB_H
#define DATABASED_HITTER_DB_H

#include <unordered_map>
#include "DbBase.h"
#include "InstrumentDB.h"
#include "Hitter.pb.h"

class HitterDB : public DbBase {
 public:
  typedef std::unordered_map<std::string, std::shared_ptr<Proto::HitterSpec>> HitterMap;
  HitterDB(ConcurrentSqliteDB &db,
           const std::string &table_name,
           const std::string &record_table_name,
           InstrumentDB &instrument_db);

 private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(
      base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  base::ProtoMessagePtr OnRequest(const std::shared_ptr<Proto::HitterReq> &msg);

  static int Callback(void *data, int argc, char **argv, char **col_name);
  static int RecordCallback(void *data, int argc, char **argv, char **col_name);

  std::string record_table_name_;
  InstrumentDB &instrument_db_;

  HitterMap hitters_;
  // HitterRecordMap records_;
};

#endif // DATABASED_HITTER_DB_H
