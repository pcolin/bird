#include "EnvConfig.h"
#include "base/logger/Logging.h"

#include <stdlib.h>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

using namespace base;

EnvConfig* EnvConfig::GetInstance() {
  static EnvConfig config;
  return &config;
}

EnvConfig::EnvConfig() {
  configs_ = {
    { static_cast<int8_t>(EnvVar::APP_NAME), "APP_NAME" },
    { static_cast<int8_t>(EnvVar::NIC), "NIC" },
    { static_cast<int8_t>(EnvVar::CONFIG_FILE), "CONFIG_FILE" },
    /// nanomsg
    { static_cast<int8_t>(EnvVar::SUB_ADDR), "SUB_ADDR" },
    { static_cast<int8_t>(EnvVar::REQ_ADDR), "REQ_ADDR" },
    { static_cast<int8_t>(EnvVar::BIND_SUB_ADDR), "BIND_SUB_ADDR" },
    { static_cast<int8_t>(EnvVar::BIND_REQ_ADDR), "BIND_REQ_ADDR" },
    /// database
    { static_cast<int8_t>(EnvVar::CONFIG_DB_FILE), "CONFIG_DB_FILE" },
    { static_cast<int8_t>(EnvVar::ORDER_DB_FILE), "ORDER_DB_FILE" },
    { static_cast<int8_t>(EnvVar::TRADE_DB_FILE), "TRADE_DB_FILE" },
    { static_cast<int8_t>(EnvVar::DEL_EXPIRE_INST), "DEL_EXPIRE_INST" },
    { static_cast<int8_t>(EnvVar::DEL_POS_AT_INIT), "DEL_POS_AT_INIT" },

    /// Logging
    { static_cast<int8_t>(EnvVar::LOGGING_LEVEL), "LOGGING_LEVEL" },
    { static_cast<int8_t>(EnvVar::LOGGING_DIR), "LOGGING_DIR" },
    { static_cast<int8_t>(EnvVar::ASYNC_LOGGING), "ASYNC_LOGGING" },
    { static_cast<int8_t>(EnvVar::ASYNC_FLUSH_INT), "ASYNC_FLUSH_INT" },

    /// exchange
    { static_cast<int8_t>(EnvVar::EXCHANGE), "EXCHANGE" },
    { static_cast<int8_t>(EnvVar::SIMULATED_EXCHANGE), "SIMULATED_EXCHANGE" },
    { static_cast<int8_t>(EnvVar::RECONNECT_INTERVAL), "RECONNECT_INT" },
    { static_cast<int8_t>(EnvVar::PRICE_TIMEOUT), "PRICE_TIMEOUT" },
    { static_cast<int8_t>(EnvVar::PUB_PRICE_LEVELS), "PUB_PRICE_LVLS" },
    { static_cast<int8_t>(EnvVar::PUB_PRICE_INTERVAL), "PUB_PRICE_INT"},
    { static_cast<int8_t>(EnvVar::QRY_CASH_INTERVAL), "QRY_CASH_INT" },
    { static_cast<int8_t>(EnvVar::OPT_CASH_LIMIT), "OPT_CASH_LIMIT" },
    { static_cast<int8_t>(EnvVar::SPOT_CASH_LIMIT), "SPOT_CASH_LIMIT" },
    { static_cast<int8_t>(EnvVar::WASH_TRADE_PROT), "WASH_TRADE_PROT" },
    { static_cast<int8_t>(EnvVar::ORDER_GC_THRESHOLD), "GC_THRESHOLD" },
    { static_cast<int8_t>(EnvVar::NIGHT_SESSION_TIME), "NIGHT_SESSION_TIME" },
    { static_cast<int8_t>(EnvVar::CLOSE_TODAY_POS), "CLOSE_TODAY_POS" },
    { static_cast<int8_t>(EnvVar::SUPPORT_QUOTE), "SUPPORT_QUOTE" },
    { static_cast<int8_t>(EnvVar::AMEND_QUOTE), "AMEND_QUOTE" },
    { static_cast<int8_t>(EnvVar::SIDE_QUOTE), "SIDE_QUOTE" },

    /// ctp
    { static_cast<int8_t>(EnvVar::CTP_TRADE_ADDR), "CTP_TRADE_ADDR" },
    { static_cast<int8_t>(EnvVar::CTP_MD_ADDR), "CTP_MD_ADDR" },
    { static_cast<int8_t>(EnvVar::CTP_BROKER_ID), "CTP_BROKER_ID" },
    { static_cast<int8_t>(EnvVar::CTP_INVESTOR_ID), "CTP_INVESTOR_ID" },
    { static_cast<int8_t>(EnvVar::CTP_USER_ID), "CTP_USER_ID" },
    { static_cast<int8_t>(EnvVar::CTP_PASSWORD), "CTP_PASSWORD" },

    { static_cast<int8_t>(EnvVar::UL_PRICE_CHECK_NUM), "UL_PRICE_CHECK_NUM" },

  };
}

bool EnvConfig::GetValue(EnvVar var, std::string &value) const {
  auto it = configs_.find(static_cast<int8_t>(var));
  if (it != configs_.end()) {
    char *v = getenv(it->second.c_str());
    if (v) {
      value = v;
      return true;
    }
  }
  return false;
}

bool EnvConfig::GetValue(EnvVar var, bool &value) const {
  auto it = configs_.find(static_cast<int8_t>(var));
  if (it != configs_.end()) {
    char *v = getenv(it->second.c_str());
    if (v) {
      value = boost::iequals(v, "TRUE");
      // value = boost::is_iequal(v, "TRUE");
      return true;
    }
  }
  return false;
}

bool EnvConfig::GetValue(EnvVar var, int32_t &value) const {
  auto it = configs_.find(static_cast<int8_t>(var));
  if (it != configs_.end()) {
    char *v = getenv(it->second.c_str());
    if (v) {
      value = atoi(v);
      return true;
    }
  }
  return false;
}

bool EnvConfig::GetValue(EnvVar var, char &value) const {
  auto it = configs_.find(static_cast<int8_t>(var));
  if (it != configs_.end()) {
    char *v = getenv(it->second.c_str());
    if (v) {
      value = v[0];
      return true;
    }
  }
  return false;
}

bool EnvConfig::GetValue(EnvVar var, double &value) const {
  auto it = configs_.find(static_cast<int8_t>(var));
  if (it != configs_.end()) {
    char *v = getenv(it->second.c_str());
    if (v) {
      value = atof(v);
      return true;
    }
  }
  return false;
}

template <class T> T EnvConfig::GetValue(EnvVar var) const {
  T ret;
  auto it = configs_.find(static_cast<int8_t>(var));
  if (it != configs_.end()) {
    if (GetValue(var, ret)) {
      LOG_INF << boost::format("%1%=%2%") % it->second % ret;
    } else {
      throw std::runtime_error(std::string("Can't read env var ") + it->second);
    }
  } else {
    throw std::runtime_error(std::string("Can't find env var ") +
          std::to_string(static_cast<int8_t>(var)));
  }
  return ret;
}

double EnvConfig::GetDouble(EnvVar var) const { return GetValue<double>(var); }

int32_t EnvConfig::GetInt32(EnvVar var) const { return GetValue<int32_t>(var); }

bool EnvConfig::GetBool(EnvVar var) const { return GetValue<bool>(var); }

std::string EnvConfig::GetString(EnvVar var) const { return GetValue<std::string>(var); }

template <class T> T EnvConfig::GetValue(EnvVar var, const T &default_value) const {
  T ret;
  auto it = configs_.find(static_cast<int8_t>(var));
  if (it != configs_.end()) {
    if (GetValue(var, ret)) {
      LOG_INF << boost::format("%1%=%2%") % it->second % ret;
    } else {
      LOG_INF << boost::format("%1% isn't configured, use default value %2%") %
                 it->second % default_value;
      ret = default_value;
    }
  } else {
    LOG_INF << boost::format("%1% isn't found, use default value %2%") %
               static_cast<int8_t>(var) % default_value;
    ret = default_value;
  }
  return ret;
}

double EnvConfig::GetDouble(EnvVar var, double default_value) const {
  return GetValue<double>(var, default_value);
}

int32_t EnvConfig::GetInt32(EnvVar var, int32_t default_value) const {
  return GetValue<int32_t>(var, default_value);
}

bool EnvConfig::GetBool(EnvVar var, bool default_value) const {
  return GetValue<bool>(var, default_value);
}

std::string EnvConfig::GetString(EnvVar var, const std::string& default_value) const {
  return GetValue<std::string>(var, default_value);
}
