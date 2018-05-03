#include "ExchangeParameterDB.h"
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"
#include "model/Message.h"

#include "boost/format.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

ExchangeParameterDB::ExchangeParameterDB(ConcurrentSqliteDB &db, const std::string &table_name,
    const std::string &holiday_table_name)
  : DbBase(db, table_name), holiday_table_name_(holiday_table_name)
    // trading_day_(boost::gregorian::day_clock::local_day())
{}

void ExchangeParameterDB::RefreshCache()
{
  char sql[1024];
  const char *fmt = "%Y%m%d";
  sprintf(sql, "DELETE FROM %s WHERE holiday<strftime(\"%s\", 'now')", holiday_table_name_.c_str(),
      fmt);
  ExecSql(sql);

  sprintf(sql, "SELECT * FROM %s", table_name_.c_str());
  ExecSql(sql, &cache_, &ExchangeParameterDB::ParameterCallback);
  sprintf(sql, "SELECT * FROM %s", holiday_table_name_.c_str());
  ExecSql(sql, &cache_, &ExchangeParameterDB::HolidayCallback);

  auto trading_day = boost::gregorian::day_clock::local_day();

  if (cache_)
  {
    const auto night_session_time = boost::posix_time::duration_from_string(
        EnvConfig::GetInstance()->GetString(EnvVar::NIGHT_SESSION_TIME, "23:59:59.0"));
    if (boost::posix_time::second_clock::local_time().time_of_day() > night_session_time)
    {
      trading_day += boost::gregorian::date_duration(1);
      auto &holidays = cache_->holidays();
      while (trading_day.day_of_week() == boost::gregorian::Saturday ||
             trading_day.day_of_week() == boost::gregorian::Sunday ||
             std::find_if(holidays.begin(), holidays.end(), [&](const auto &h)
               {
                 return h.date() == boost::gregorian::to_iso_string(trading_day);
               }) != holidays.end())
      {
        trading_day += boost::gregorian::date_duration(1);
      }
      cache_->set_night_session(true);
    }
  }
  trading_day_ = boost::gregorian::to_iso_string(trading_day);
  cache_->set_trading_day(trading_day_);
  LOG_INF << "Trading day is " << trading_day_;
}

void ExchangeParameterDB::RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher)
{
  dispatcher.RegisterCallback<Proto::ExchangeParameterReq>(
    std::bind(&ExchangeParameterDB::OnRequest, this, std::placeholders::_1));
}

base::ProtoMessagePtr ExchangeParameterDB::OnRequest(
    const std::shared_ptr<Proto::ExchangeParameterReq> &msg)
{
  LOG_INF << "Exchange parameter request: " << msg->ShortDebugString();
  auto reply = Message::NewProto<Proto::ExchangeParameterRep>();
  if (msg->type() == Proto::RequestType::Get)
  {
    if (cache_)
    {
      reply->add_parameters()->CopyFrom(*cache_);
    }
    else
    {
      reply->mutable_result()->set_result(false);
      reply->mutable_result()->set_error("no exchange parameter");
      return reply;
    }
  }
  else if (msg->type() == Proto::RequestType::Set)
  {
    char sql[1024];
    TransactionGuard tg(this);

    for (auto &p : msg->parameters())
    {
      auto ep = Message::NewProto<Proto::ExchangeParameter>();
      ep->CopyFrom(p);
      cache_.swap(ep);

      std::ostringstream oss;
      for (auto &s : p.sessions())
      {
        oss << s.begin() << '-' << s.end() << ',';
      }
      std::ostringstream oss1;
      for (auto &s : p.maturity_sessions())
      {
        oss1 << s.begin() << '-' << s.end() << ',';
      }
      sprintf(sql, "INSERT OR REPLACE INTO %s VALUES(%d, '%s', '%s', '%s', %d, %d, %d)",
          table_name_.c_str(), static_cast<int32_t>(p.exchange()),
          oss.str().c_str(), oss1.str().c_str(), p.charm_start_time().c_str(),
          p.rfq_delay(), p.rfq_timeout(), p.rfq_volume());
      ExecSql(sql);

      sprintf(sql, "DELETE FROM %s", holiday_table_name_.c_str());
      ExecSql(sql);
      for (auto &h : p.holidays())
      {
        sprintf(sql, "INSERT INTO %s VALUES('%s', %f)", holiday_table_name_.c_str(),
            h.date().c_str(), h.weight());
        ExecSql(sql);
      }
    }
  }
  else if (msg->type() == Proto::RequestType::Del)
  {
    cache_.reset();
    char sql[1024];
    TransactionGuard tg(this);
    sprintf(sql, "DELETE FROM %s", table_name_.c_str());
    ExecSql(sql);
    sprintf(sql, "DELETE FROM %s", holiday_table_name_.c_str());
    ExecSql(sql);
  }
  reply->mutable_result()->set_result(true);
  return reply;
}

int ExchangeParameterDB::ParameterCallback(void *data, int argc, char **argv, char **col_name)
{
  auto p = Message::NewProto<Proto::ExchangeParameter>();
  p->set_exchange(static_cast<Proto::Exchange>(atoi(argv[0])));
  ParseTradingSession(argv[1], std::bind(&Proto::ExchangeParameter::add_sessions, p.get()));
  ParseTradingSession(argv[2], std::bind(&Proto::ExchangeParameter::add_maturity_sessions, p.get()));
  p->set_charm_start_time(argv[3]);
  p->set_rfq_delay(atoi(argv[4]));
  p->set_rfq_timeout(atoi(argv[5]));
  p->set_rfq_volume(atoi(argv[6]));

  auto &cache = *static_cast<std::shared_ptr<Proto::ExchangeParameter>*>(data);
  cache = std::move(p);
  return 0;
}

void ExchangeParameterDB::ParseTradingSession(char *data,
    std::function<Proto::TradingSession*()> func)
{
  char *beg = data, *mid = data, *pos = data + 1;
  while (*pos != 0)
  {
    if (*pos == '-')
    {
      mid = pos;
    }
    else if (*pos == ',')
    {
      auto *s = func();
      s->set_begin(beg, mid - beg);
      s->set_end(mid + 1, pos - mid - 1);
      beg = mid = pos + 1;
    }
    ++pos;
  }
}

int ExchangeParameterDB::HolidayCallback(void *data, int argc, char **argv, char **col_name)
{
  auto &cache = *static_cast<std::shared_ptr<Proto::ExchangeParameter>*>(data);
  auto *holiday = cache->add_holidays();
  holiday->set_date(argv[0]);
  holiday->set_weight(atof(argv[1]));
  return 0;
}
