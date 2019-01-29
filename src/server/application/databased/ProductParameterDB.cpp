#include "ProductParameterDB.h"
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"
#include "model/Message.h"
#include "boost/format.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

ProductParameterDB::ProductParameterDB(ConcurrentSqliteDB &db,
                                         const std::string &table_name,
                                         const std::string &holiday_table_name)
    : DbBase(db, table_name), holiday_table_name_(holiday_table_name) {}

void ProductParameterDB::RefreshCache() {
  char sql[1024];
  auto today = boost::gregorian::day_clock::local_day();
  today -= boost::gregorian::date_duration(30);
  sprintf(sql, "DELETE FROM %s WHERE holiday<%s", holiday_table_name_.c_str(),
          boost::gregorian::to_iso_string(today).c_str());
  ExecSql(sql);

  sprintf(sql, "SELECT * FROM %s", table_name_.c_str());
  ExecSql(sql, &cache_, &ProductParameterDB::ParameterCallback);
  sprintf(sql, "SELECT * FROM %s", holiday_table_name_.c_str());
  ExecSql(sql, &cache_, &ProductParameterDB::HolidayCallback);

  SetTradingDay();
}

void ProductParameterDB::RegisterCallback(
    base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher) {
  dispatcher.RegisterCallback<Proto::ProductParameterReq>(
      std::bind(&ProductParameterDB::OnRequest, this, std::placeholders::_1));
}

base::ProtoMessagePtr ProductParameterDB::OnRequest(
    const std::shared_ptr<Proto::ProductParameterReq> &msg) {
  LOG_INF << "product parameter request: " << msg->ShortDebugString();
  auto reply = Message::NewProto<Proto::ProductParameterRep>();
  if (msg->type() == Proto::RequestType::Get) {
    if (msg->product().empty()) {
      for (auto it : cache_) {
        reply->add_parameters()->CopyFrom(*it.second);
      }
    } else {
      auto it = cache_.find(msg->product());
      if (it != cache_.end()) {
        reply->add_parameters()->CopyFrom(*it->second);
      } else {
        reply->mutable_result()->set_result(false);
        reply->mutable_result()->set_error(msg->product() + " doesn't exist");
        return reply;
      }
    }
  } else if (msg->type() == Proto::RequestType::Set) {
    char sql[1024];
    TransactionGuard tg(this);

    for (auto &p : msg->parameters()) {
      auto tmp = Message::NewProto<Proto::ProductParameter>();
      tmp->CopyFrom(p);
      cache_[p.product()] = tmp;
      SetTradingDay();

      std::ostringstream oss;
      for (auto &s : p.sessions()) {
        oss << s.begin() << '-' << s.end() << '-' << s.stop() << ',';
      }
      std::ostringstream oss1;
      for (auto &s : p.maturity_sessions()) {
        oss1 << s.begin() << '-' << s.end() << '-' << s.stop() << ',';
      }
      sprintf(sql, "INSERT OR REPLACE INTO %s VALUES('%s', %d, '%s', '%s', '%s', %d, %d, %d)",
              table_name_.c_str(), p.product().c_str(), static_cast<int32_t>(p.exchange()),
              oss.str().c_str(), oss1.str().c_str(), p.charm_start_time().c_str(),
              p.rfq_delay(), p.rfq_timeout(), p.rfq_volume());
      ExecSql(sql);

      sprintf(sql, "DELETE FROM %s", holiday_table_name_.c_str());
      ExecSql(sql);
      for (auto &h : p.holidays()) {
        sprintf(sql, "INSERT INTO %s VALUES('%s', '%s', %f)", holiday_table_name_.c_str(),
                p.product().c_str(), h.date().c_str(), h.weight());
        ExecSql(sql);
      }
    }
  } else if (msg->type() == Proto::RequestType::Del) {
    char sql[1024];
    TransactionGuard tg(this);

    for (auto &p : msg->parameters()) {
      sprintf(sql, "DELETE FROM %s where product=%s", table_name_.c_str(),
              p.product().c_str());
      ExecSql(sql);
      sprintf(sql, "DELETE FROM %s where product=%s", holiday_table_name_.c_str(),
              p.product().c_str());
      ExecSql(sql);
    }
  }
  reply->mutable_result()->set_result(true);
  return reply;
}

void ProductParameterDB::SetTradingDay() {
  auto trading_day = boost::gregorian::day_clock::local_day();
  const auto time = EnvConfig::GetInstance()->GetString(EnvVar::NIGHT_SESSION_TIME,
                                                        "23:59:59.0");
  for (auto it : cache_) {
    if (trading_day_.empty()) {
      const auto night_time = boost::posix_time::duration_from_string(time);
      if (boost::posix_time::second_clock::local_time().time_of_day() > night_time) {
        trading_day += boost::gregorian::date_duration(1);
      }
      auto &holidays = it.second->holidays();
      while (trading_day.day_of_week() == boost::gregorian::Saturday ||
             trading_day.day_of_week() == boost::gregorian::Sunday ||
             std::find_if(holidays.begin(), holidays.end(), [&](const auto &h)
               {
                 return h.date() == boost::gregorian::to_iso_string(trading_day);
               }) != holidays.end()) {
        trading_day += boost::gregorian::date_duration(1);
      }
      trading_day_ = boost::gregorian::to_iso_string(trading_day);
      LOG_INF << "trading day is " << trading_day_;
    }
    it.second->set_night_session_time(time);
    it.second->set_trading_day(trading_day_);
  }
}

int ProductParameterDB::ParameterCallback(void *data, int argc, char **argv, char **col_name) {
  auto p = Message::NewProto<Proto::ProductParameter>();
  p->set_product(argv[0]);
  p->set_exchange(static_cast<Proto::Exchange>(atoi(argv[1])));
  ParseTradingSession(argv[2], std::bind(&Proto::ProductParameter::add_sessions, p.get()));
  ParseTradingSession(argv[3], std::bind(&Proto::ProductParameter::add_maturity_sessions, p.get()));
  p->set_charm_start_time(argv[4]);
  p->set_rfq_delay(atoi(argv[5]));
  p->set_rfq_timeout(atoi(argv[6]));
  p->set_rfq_volume(atoi(argv[7]));

  auto &cache = *static_cast<ProductParameterMap*>(data);
  cache[p->product()] = std::move(p);
  return 0;
}

void ProductParameterDB::ParseTradingSession(char *data,
                                             std::function<Proto::TradingSession*()> func) {
  char *beg = data, *first = data, *second = data, *pos = data + 1;
  while (*pos != 0) {
    if (*pos == '-') {
      if (first == beg) {
        first = pos;
      } else {
        second = pos;
      }
    } else if (*pos == ',') {
      auto *s = func();
      s->set_begin(beg, first - beg);
      s->set_end(first + 1, second - first - 1);
      s->set_stop(second + 1, pos - second - 1);
      beg = first = second = pos + 1;
    }
    ++pos;
  }
}

int ProductParameterDB::HolidayCallback(void *data,
                                        int argc,
                                        char **argv,
                                        char **col_name) {
  auto &cache = *static_cast<ProductParameterMap*>(data);
  const std::string product = argv[0];
  auto it = cache.find(product);
  if (it != cache.end()) {
    auto *holiday = it->second->add_holidays();
    holiday->set_date(argv[1]);
    holiday->set_weight(atof(argv[2]));
  }
  return 0;
}
