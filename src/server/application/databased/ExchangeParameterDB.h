#ifndef DATABASED_EXCHANGE_PARAMETER_DB_H
#define DATABASED_EXCHANGE_PARAMETER_DB_H

#include "DbBase.h"
#include "ExchangeParameter.pb.h"

#include <unordered_map>
#include <boost/date_time/gregorian/gregorian.hpp>

class ExchangeParameterDB : public DbBase
{
public:
  ExchangeParameterDB(ConcurrentSqliteDB &db, const std::string &table_name,
      const std::string &holiday_table_name);

  const boost::gregorian::date& TradingDay() const { return trading_day_; }

private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  base::ProtoMessagePtr OnRequest(const std::shared_ptr<Proto::ExchangeParameterReq> &msg);

  static int ParameterCallback(void *data, int argc, char **argv, char **col_name);
  static int HolidayCallback(void *data, int argc, char **argv, char **col_name);
  static void ParseTradingSession(char *data, std::function<Proto::TradingSession*()> func);

  std::shared_ptr<Proto::ExchangeParameter> cache_;
  std::string holiday_table_name_;
  boost::gregorian::date trading_day_;
};

#endif
