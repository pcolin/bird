#ifndef DATABASED_INSTRUMENT_DB_H
#define DATABASED_INSTRUMENT_DB_H

#include <unordered_map>
#include "DbBase.h"
#include "ProductParameterDB.h"
#include "Instrument.pb.h"

class InstrumentDB : public DbBase {
 public:
  typedef std::unordered_map<std::string, std::shared_ptr<Proto::Instrument>> InstrumentMap;
  InstrumentDB(ConcurrentSqliteDB &db, const std::string &table_name, ProductParameterDB &product_db);
  std::shared_ptr<Proto::Instrument> FindOption(const std::string &id);
  std::shared_ptr<Proto::Instrument> FindUnderlying(const std::string &id);
  std::shared_ptr<Proto::Instrument> FindInstrument(const std::string &id);

 private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  base::ProtoMessagePtr OnRequest(const std::shared_ptr<Proto::InstrumentReq> &msg);
  void UpdateInstrument(const Proto::Instrument &inst, InstrumentMap &cache);
  void UpdateInstrumentStatus(const Proto::Instrument &inst, InstrumentMap &cache);

  static int UnderlyingCallback(void *data, int argc, char **argv, char **col_name);
  static int OptionCallback(void *data, int argc, char **argv, char **col_name);

  std::string trading_date_;
  InstrumentMap underlyings_;
  InstrumentMap options_;
};

#endif // DATABASED_INSTRUMENT_DB_H
