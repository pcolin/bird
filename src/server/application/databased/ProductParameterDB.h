#ifndef DATABASED_PRODUCT_PARAMETER_DB_H
#define DATABASED_PRODUCT_PARAMETER_DB_H

#include <map>
#include "DbBase.h"
#include "ProductParameter.pb.h"

class ProductParameterDB : public DbBase {
 public:
  ProductParameterDB(ConcurrentSqliteDB &db,
                      const std::string &table_name,
                      const std::string &holiday_table_name);

  const std::string& TradingDay() const { return trading_day_; }

 private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  base::ProtoMessagePtr OnRequest(const std::shared_ptr<Proto::ProductParameterReq> &msg);
  void SetTradingDay();

  static int ParameterCallback(void *data, int argc, char **argv, char **col_name);
  static int HolidayCallback(void *data, int argc, char **argv, char **col_name);
  static void ParseTradingSession(char *data,
                                  std::function<Proto::TradingSession*()> func);

  typedef std::map<std::string,
                   std::shared_ptr<Proto::ProductParameter>> ProductParameterMap;
  ProductParameterMap cache_;
  std::string holiday_table_name_;
  std::string trading_day_;
  // boost::gregorian::date trading_day_;
};

#endif // DATABASED_PRODUCT_PARAMETER_DB_H
