#include "TradeDB.h"
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"
#include "model/Message.h"
#include "boost/format.hpp"

TradeDB::TradeDB(ConcurrentSqliteDB &db,
                 InstrumentDB &instrument_db,
                 ExchangeParameterDB &exchange_db)
    : DbBase(db, "Trade" + exchange_db.TradingDay()),
      requests_(kCapacity),
      instrument_db_(instrument_db) {
  thread_ = std::thread(std::bind(&TradeDB::Run, this));
}

void TradeDB::RefreshCache() {
  char sql[1024];
  sprintf(sql, "CREATE TABLE '%s'(id varchar(24) PRIMARY KEY UNIQUE, instrument varchar(20), "
               "exchange integer, side integer, price real, volume integer, time varchar(30), "
               "order_id varchar(24))",
          table_name_.c_str());
  if (!ExecSql(sql)) {
    sprintf(sql, "SELECT * FROM %s", table_name_.c_str());
    auto data = std::make_tuple(&trades_, &instrument_db_);
    ExecSql(sql, &data, TradeDB::Callback);
  }
}

void TradeDB::RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher) {
  dispatcher.RegisterCallback<Proto::TradeReq>(
      std::bind(&TradeDB::OnRequest, this, std::placeholders::_1));
  dispatcher.RegisterCallback<Proto::Trade>(
      std::bind(&TradeDB::OnTrade, this, std::placeholders::_1));
}

base::ProtoMessagePtr TradeDB::OnRequest(const std::shared_ptr<Proto::TradeReq> &msg) {
  LOG_INF << "Trade request: " << msg->ShortDebugString();
  auto reply = Message::NewProto<Proto::TradeRep>();
  if (msg->type() == Proto::RequestType::Get) {
    std::lock_guard<std::mutex> lck(mtx_);
    for (auto &trade : trades_) {
      reply->add_trades()->CopyFrom(*trade);
    }
    LOG_INF << boost::format("Get %1% trades totally.") % reply->trades_size();
  }
  // else
  // {
  //   requests_.enqueue(msg);
  // }
  reply->mutable_result()->set_result(true);
  return reply;
}

base::ProtoMessagePtr TradeDB::OnTrade(const std::shared_ptr<Proto::Trade> &msg) {
  LOG_INF << "Trade: " << msg->ShortDebugString();
  requests_.enqueue(msg);
  return nullptr;
}

void TradeDB::Run() {
  LOG_INF << "Trade update thread is running...";
  std::shared_ptr<Proto::Trade> trades[kCapacity];
  while (true) {
    size_t cnt = requests_.wait_dequeue_bulk(trades, kCapacity);
    char sql[1024];
    TransactionGuard tg(this);
    for (size_t i = 0; i < cnt; ++i) {
      char time[32];
      base::TimeToString(trades[i]->time(), time, sizeof(time));
      sprintf(sql, "INSERT OR REPLACE INTO %s VALUES('%s', '%s', %d, %d, %f, %d, '%s', '%s')",
              table_name_.c_str(), trades[i]->id().c_str(), trades[i]->instrument().c_str(),
              static_cast<int>(trades[i]->exchange()), static_cast<int>(trades[i]->side()),
              trades[i]->price(), trades[i]->volume(), time,
              std::to_string(trades[i]->order_id()).c_str());
      ExecSql(sql);
      {
        std::lock_guard<std::mutex> lck(mtx_);
        trades_.push_back(trades[i]);
      }
    }
  }
}

int TradeDB::Callback(void *data, int argc, char **argv, char **col_name) {
  auto *tmp = static_cast<std::tuple<TradeArray*, InstrumentDB*>*>(data);
  auto *trades = std::get<0>(*tmp);
  auto *instrument_db = std::get<1>(*tmp);

  const std::string instrument = argv[1];
  auto inst = instrument_db->FindInstrument(instrument);
  if (inst) {
    auto trade = Message::NewProto<Proto::Trade>();
    const std::string id = argv[0];
    trade->set_id(id);
    trade->set_instrument(instrument);
    trade->set_exchange(static_cast<Proto::Exchange>(atoi(argv[2])));
    trade->set_side(static_cast<Proto::Side>(atoi(argv[3])));
    trade->set_price(atof(argv[4]));
    trade->set_volume(atoi(argv[5]));
    trade->set_time(base::StringToTime(argv[6]));
    char *end = NULL;
    trade->set_order_id(strtoull(argv[7], &end, 10));
    trades->push_back(trade);
  } else {
    LOG_ERR << "Can't find instrument " << instrument;
  }
  return 0;
}
