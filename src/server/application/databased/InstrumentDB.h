#ifndef DATABASED_INSTRUMENT_DB_H
#define DATABASED_INSTRUMENT_DB_H

#include "DbBase.h"
#include "ExchangeParameterDB.h"
#include "Instrument.pb.h"

#include <unordered_map>

class InstrumentDB : public DbBase
{
  typedef std::unordered_map<std::string, std::shared_ptr<Proto::Instrument>> InstrumentMap;
public:
  InstrumentDB(ConcurrentSqliteDB &db, const std::string &table_name, ExchangeParameterDB &exch);
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

#endif
