#include "EnvConfig.h"
#include "base/logger/Logging.h"

#include <stdlib.h>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

using namespace base;

EnvConfig* EnvConfig::GetInstance()
{
  static EnvConfig config;
  return &config;
}

EnvConfig::EnvConfig()
{
  configs_ =
  {
    { static_cast<int8_t>(EnvVar::APP_NAME), "APP_NAME" },
    { static_cast<int8_t>(EnvVar::NIC), "NIC" },
    { static_cast<int8_t>(EnvVar::CONFIG_FILE), "CONFIG_FILE" },
    /// database
    { static_cast<int8_t>(EnvVar::CONFIG_DB_FILE), "CONFIG_DB_FILE" },
    { static_cast<int8_t>(EnvVar::ORDER_DB_FILE), "ORDER_DB_FILE" },
    { static_cast<int8_t>(EnvVar::TRADE_DB_FILE), "TRADE_DB_FILE" },

    /// Logging
    { static_cast<int8_t>(EnvVar::LOGGING_LEVEL), "LOGGING_LEVEL" },
    { static_cast<int8_t>(EnvVar::LOGGING_DIR), "LOGGING_DIR" },
    { static_cast<int8_t>(EnvVar::ASYNC_LOGGING), "ASYNC_LOGGING" },
    { static_cast<int8_t>(EnvVar::ASYNC_FLUSH_INT), "ASYNC_FLUSH_INT" },

    /// ctp
    { static_cast<int8_t>(EnvVar::CTP_TRADE_ADDR), "CTP_TRADE_ADDR" },
    { static_cast<int8_t>(EnvVar::CTP_MD_ADDR), "CTP_MD_ADDR" },
    { static_cast<int8_t>(EnvVar::CTP_BROKER_ID), "CTP_BROKER_ID" },
    { static_cast<int8_t>(EnvVar::CTP_INVESTOR_ID), "CTP_INVESTOR_ID" },
    { static_cast<int8_t>(EnvVar::CTP_USER_ID), "CTP_USER_ID" },
    { static_cast<int8_t>(EnvVar::CTP_PASSWORD), "CTP_PASSWORD" },

  };
}

bool EnvConfig::GetValue(EnvVar var, std::string &value) const
{
  auto it = configs_.find(static_cast<int8_t>(var));
  if (it != configs_.end())
  {
    char *v = getenv(it->second.c_str());
    if (v)
    {
      value = v;
      return true;
    }
  }
  return false;
}

bool EnvConfig::GetValue(EnvVar var, bool &value) const
{
  auto it = configs_.find(static_cast<int8_t>(var));
  if (it != configs_.end())
  {
    char *v = getenv(it->second.c_str());
    if (v)
    {
      value = boost::iequals(v, "TRUE");
      // value = boost::is_iequal(v, "TRUE");
      return true;
    }
  }
  return false;
}

bool EnvConfig::GetValue(EnvVar var, int32_t &value) const
{
  auto it = configs_.find(static_cast<int8_t>(var));
  if (it != configs_.end())
  {
    char *v = getenv(it->second.c_str());
    if (v)
    {
      value = atoi(v);
      return true;
    }
  }
  return false;
}

bool EnvConfig::GetValue(EnvVar var, char &value) const
{
  auto it = configs_.find(static_cast<int8_t>(var));
  if (it != configs_.end())
  {
    char *v = getenv(it->second.c_str());
    if (v)
    {
      value = v[0];
      return true;
    }
  }
  return false;
}

bool EnvConfig::GetValue(EnvVar var, double &value) const
{
  auto it = configs_.find(static_cast<int8_t>(var));
  if (it != configs_.end())
  {
    char *v = getenv(it->second.c_str());
    if (v)
    {
      value = atof(v);
      return true;
    }
  }
  return false;
}

template <class T> T EnvConfig::GetValue(EnvVar var) const
{
  T ret;
  auto it = configs_.find(static_cast<int8_t>(var));
  if (it != configs_.end())
  {
    if (GetValue(var, ret))
    {
      LOG_INF << boost::format("%1%=%2%") % it->second % ret;
    }
    else
    {
      throw std::runtime_error(std::string("Can't read env var ") + it->second);
    }
  }
  else
  {
    throw std::runtime_error(std::string("Can't find env var ") +
        std::to_string(static_cast<int8_t>(var)));
  }
  return ret;
}

double EnvConfig::GetDouble(EnvVar var) const
{
  return GetValue<double>(var);
}

int32_t EnvConfig::GetInt32(EnvVar var) const
{
  return GetValue<int32_t>(var);
}

bool EnvConfig::GetBool(EnvVar var) const
{
  return GetValue<bool>(var);
}

std::string EnvConfig::GetString(EnvVar var) const
{
  return GetValue<std::string>(var);
}

template <class T> T EnvConfig::GetValue(EnvVar var, const T &default_value) const
{
  T ret;
  auto it = configs_.find(static_cast<int8_t>(var));
  if (it != configs_.end())
  {
    if (GetValue(var, ret))
    {
      LOG_INF << boost::format("%1%=%2%") % it->second % ret;
    }
    else
    {
      LOG_INF << boost::format("%1% isn't configured, use default value %2%") %
        it->second % default_value;
      ret = default_value;
    }
  }
  else
  {
    LOG_INF << boost::format("%1% isn't found, use default value %2%") %
      static_cast<int8_t>(var) % default_value;
    ret = default_value;
  }
  return ret;
}

double EnvConfig::GetDouble(EnvVar var, double default_value) const
{
  return GetValue<double>(var, default_value);
}

int32_t EnvConfig::GetInt32(EnvVar var, int32_t default_value) const
{
  return GetValue<int32_t>(var, default_value);
}

bool EnvConfig::GetBool(EnvVar var, bool default_value) const
{
  return GetValue<bool>(var, default_value);
}

std::string EnvConfig::GetString(EnvVar var, const std::string& default_value) const
{
  return GetValue<std::string>(var, default_value);
}
