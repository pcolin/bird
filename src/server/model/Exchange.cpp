#include "Exchange.h"
#include "config/EnvConfig.h"
#include "base/logger/Logging.h"
#include "boost/format.hpp"

using namespace boost::gregorian;

void Exchange::OnExchangeParameter(const Proto::ExchangeParameter &param)
{
  LOG_INF << "Update exchange parameter: " << param.ShortDebugString();

  std::ostringstream oss;
  auto ParseSessionFunc = [&](const auto &proto_sessions, auto &sessions, int32_t &seconds)
  {
    static const auto NIGHT_SESSION_TIME = boost::posix_time::duration_from_string(
        EnvConfig::GetInstance()->GetString(EnvVar::NIGHT_SESSION_TIME, "23:59:59.0"));
    sessions.clear();
    seconds = 0;
    for (auto &s : proto_sessions)
    {
      auto d = trading_day_;
      auto begin_time = boost::posix_time::duration_from_string(s.begin());
      if (begin_time >= NIGHT_SESSION_TIME)
      {
        do
        {
          d -= date_duration(1);
        } while (d.day_of_week() == Saturday || d.day_of_week() == Sunday ||
            holidays_.find(d) != holidays_.end());
      }
      boost::posix_time::ptime begin(d, begin_time);

      auto end_time = boost::posix_time::duration_from_string(s.end());
      if (begin_time > end_time)
      {
        d += boost::gregorian::date_duration(1);
      }
      boost::posix_time::ptime end(d, end_time);
      sessions.push_back(std::make_pair(begin, end));
      // sessions.emplace(begin, end);
      oss << "[" << begin << ", " << end << "] ";
      constexpr int32_t SECONDS_PER_DAY = 24 * 3600;
      seconds += (end - begin).total_seconds() % SECONDS_PER_DAY;
    }
  };

  std::lock_guard<std::mutex> lck(mtx_);
  trading_day_ = boost::gregorian::from_undelimited_string(param.trading_day());
  LOG_INF << Proto::Exchange_Name(param.exchange()) << " trading day: " << param.trading_day();

  holidays_.clear();
  for (auto &h : param.holidays())
  {
    auto d = boost::gregorian::from_undelimited_string(h.date());
    if (d.is_special() == false)
    {
      holidays_.emplace(d, h.weight());
      LOG_INF << boost::format("Add holiday %1% with weight %2%") % h.date() % h.weight();
    }
  }

  ParseSessionFunc(param.sessions(), sessions_, session_seconds_);
  LOG_INF << Proto::Exchange_Name(param.exchange()) << " sessions: " << oss.str();
  oss.str("");
  ParseSessionFunc(param.maturity_sessions(), maturity_sessions_, maturity_session_seconds_);
  LOG_INF << Proto::Exchange_Name(param.exchange()) << " maturity sessions: " << oss.str();

  days_to_maturity_.clear();
}

double Exchange::GetTimeValue(const boost::gregorian::date &maturity)
{
  double trading_days = 0;
  double fraction = 0;

  std::lock_guard<std::mutex> lck(mtx_);
  auto it = days_to_maturity_.find(maturity);
  if (it != days_to_maturity_.end())
  {
    trading_days = it->second;
  }
  else
  {
    int32_t days = 0;
    for (auto d = trading_day_ + date_duration(1); d <= maturity; d += date_duration(1))
    {
      if (d.day_of_week() != Saturday && d.day_of_week() != Sunday) ++days;
    }

    double holidays = 0;
    auto lower = holidays_.lower_bound(trading_day_);
    if (lower != holidays_.end())
    {
      auto upper = holidays_.upper_bound(maturity);
      for (auto itr = lower; itr != upper; ++itr)
      {
        if (itr->first.day_of_week() != Saturday && itr->first.day_of_week() != Sunday)
          holidays += (1 - itr->second);
      }
    }

    trading_days = days - holidays;
    LOG_INF << boost::format("Days to maturity(%1%): days(%2%), holidays(%3%)") %
      to_iso_string(maturity) % days % holidays;
    days_to_maturity_.emplace(maturity, trading_days);
  }

  auto now = boost::posix_time::second_clock::local_time();
  auto FractionFunc = [&](const auto &sessions, int32_t seconds)
  {
    for (auto it = sessions.rbegin(); it != sessions.rend() && now < it->second; ++it)
    {
      double tmp = static_cast<double>((it->second - std::max(now, it->first)).total_seconds()) /
        seconds;
      fraction += (tmp - std::floor(tmp));
    }
  };
  if (trading_day_ < maturity)
    FractionFunc(sessions_, session_seconds_);
  else
    FractionFunc(maturity_sessions_, maturity_session_seconds_);

  double ret = (trading_days + fraction) / AnnualTradingDays;
  LOG_INF << boost::format("Time value(%1%) : %2%, trading_days(%3%), fraction(%4%)") %
    to_iso_string(maturity) % ret % trading_days % fraction;
  return ret;
}
