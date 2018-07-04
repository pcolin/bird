#ifndef DATABASED_QUOTER_DB_H
#define DATABASED_QUOTER_DB_H

#include "DbBase.h"
#include "InstrumentDB.h"
#include "Quoter.pb.h"

#include <unordered_map>

class QuoterDB : public DbBase
{
  typedef std::unordered_map<std::string, std::shared_ptr<Proto::QuoterSpec>> QuoterMap;
  // typedef std::map<std::string,
  //         std::unordered_map<std::string, std::shared_ptr<Proto::QuoterRecord>>> QuoterRecordMap;
public:
  QuoterDB(ConcurrentSqliteDB &db, const std::string &table_name,
      const std::string &record_table_name, InstrumentDB &instrument_db);

private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  base::ProtoMessagePtr OnRequest(const std::shared_ptr<Proto::QuoterReq> &msg);

  static int Callback(void *data, int argc, char **argv, char **col_name);
  static int RecordCallback(void *data, int argc, char **argv, char **col_name);

  std::string record_table_name_;
  InstrumentDB &instrument_db_;

  QuoterMap quoters_;
  // QuoterRecordMap records_;
};

#endif
