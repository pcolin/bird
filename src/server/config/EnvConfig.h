#ifndef ENV_CONFIG_H
#define ENV_CONFIG_H

#include <unordered_map>

enum class EnvVar : int8_t
{
  APP_NAME = 0,
  NIC,
  CONFIG_FILE,

  /// nanomsg
  SUB_ADDR,
  REQ_ADDR,
  BIND_SUB_ADDR,
  BIND_REQ_ADDR,

  /// database
  CONFIG_DB_FILE,
  ORDER_DB_FILE,
  TRADE_DB_FILE,

  /// Logging
  LOGGING_LEVEL,
  LOGGING_DIR,
  ASYNC_LOGGING,
  ASYNC_FLUSH_INT,

  /// exchange
  EXCHANGE,
  QRY_CASH_INTERVAL,
  OPT_CASH_LIMIT,
  SPOT_CASH_LIMIT,
  WASH_TRADE_PROT,
  ORDER_GC_THRESHOLD, 

  /// ctp
  CTP_TRADE_ADDR,
  CTP_MD_ADDR,
  CTP_BROKER_ID,
  CTP_INVESTOR_ID,
  CTP_USER_ID,
  CTP_PASSWORD,
  CTP_UL_FILE

};

class EnvConfig
{
  public:
    static EnvConfig* GetInstance();

    bool GetValue(EnvVar var, std::string &value) const;
    bool GetValue(EnvVar var, bool &value) const;
    bool GetValue(EnvVar var, int32_t &value) const;
    bool GetValue(EnvVar var, char &value) const;
    bool GetValue(EnvVar var, double &value) const;

    double GetDouble(EnvVar var) const;
    int32_t GetInt32(EnvVar var) const;
    bool GetBool(EnvVar var) const;
    std::string GetString(EnvVar var) const;

    double GetDouble(EnvVar var, double default_value) const;
    int32_t GetInt32(EnvVar var, int32_t default_value) const;
    bool GetBool(EnvVar var, bool default_value) const;
    std::string GetString(EnvVar var, const std::string& default_value) const;

  private:
    EnvConfig();
    template <class T> T GetValue(EnvVar var) const;
    template <class T> T GetValue(EnvVar var, const T &default_value) const;

    std::unordered_map<int8_t, const std::string> configs_;
};

#endif
