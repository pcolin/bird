#include "OrderDB.h"
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"
#include "model/Message.h"
#include "boost/format.hpp"

OrderDB::OrderDB(ConcurrentSqliteDB &db,
                 InstrumentDB &instrument_db,
                 ProductParameterDB &product_db)
    : DbBase(db, "Order" + product_db.TradingDay()),
      requests_(kCapacity),
      instrument_db_(instrument_db) {
  thread_ = std::thread(std::bind(&OrderDB::Run, this));
}

void OrderDB::RefreshCache() {
  char sql[1024];
  sprintf(sql, "CREATE TABLE '%s'(id varchar(24) PRIMARY KEY UNIQUE, instrument varchar(20), "
               "counter_id varchar(15), exchange_id varchar(24), strategy varchar(20), "
               "note varchar(100), price real, avg_executed_price real, volume integer, "
               "executed_volume integer, exchange integer, strategy_type integer, "
               "side integer, time_condition integer, type integer, status integer, "
               "time varchar(30), latency integer)",
          table_name_.c_str());
  if (!ExecSql(sql)) {
    sprintf(sql, "SELECT * FROM %s WHERE status < %d", table_name_.c_str(),
            static_cast<int>(Proto::OrderStatus::Canceled));
    auto data = std::make_tuple(&orders_, &instrument_db_);
    ExecSql(sql, &data, OrderDB::Callback);
  }
}

void OrderDB::RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher) {
  dispatcher.RegisterCallback<Proto::OrderReq>(
      std::bind(&OrderDB::OnRequest, this, std::placeholders::_1));
}

base::ProtoMessagePtr OrderDB::OnRequest(const std::shared_ptr<Proto::OrderReq> &msg) {
  LOG_INF << "Order request: " << msg->ShortDebugString();
  auto reply = std::make_shared<Proto::OrderRep>();
  if (msg->type() == Proto::RequestType::Get) {
    std::lock_guard<std::mutex> lck(mtx_);
    for (auto &order : orders_) {
      reply->add_orders()->CopyFrom(*order.second);
    }
    LOG_INF << boost::format("Get %1% orders totally.") % reply->orders_size();
  } else {
    requests_.enqueue(msg);
  }
  reply->mutable_result()->set_result(true);
  return reply;
}

void OrderDB::Run() {
  LOG_INF << "Order update thread is running...";
  std::shared_ptr<Proto::OrderReq> requests[kCapacity];
  while (true) {
    size_t cnt = requests_.wait_dequeue_bulk(requests, kCapacity);
    char sql[1024];
    TransactionGuard tg(this);
    for (size_t i = 0; i < cnt; ++i) {
      if (requests[i]->type() == Proto::RequestType::Set) {
        for (auto &ord : requests[i]->orders()) {
          char time[32];
          base::TimeToString(ord.time(), time, sizeof(time));
          // sprintf(sql, "INSERT OR REPLACE INTO %s VALUES(%llu, '%s', '%s', '%s', '%s', '%s', %f, "
          sprintf(sql, "INSERT OR REPLACE INTO %s VALUES('%s', '%s', '%s', '%s', '%s', '%s', %f, "
                       "%f, %d, %d, %d, %d, %d, %d, %d, %d, '%s', %d)",
                  table_name_.c_str(), std::to_string(ord.id()).c_str(), ord.instrument().c_str(),
                  ord.counter_id().c_str(), ord.exchange_id().c_str(), ord.strategy().c_str(),
                  ord.note().c_str(), ord.price(), ord.avg_executed_price(), ord.volume(),
                  ord.executed_volume(), static_cast<int>(ord.exchange()),
                  static_cast<int>(ord.strategy_type()), static_cast<int>(ord.side()),
                  static_cast<int>(ord.time_condition()), static_cast<int>(ord.type()),
                  static_cast<int>(ord.status()), time, ord.latency());
          ExecSql(sql);
          if (ord.status() == Proto::OrderStatus::Canceled) {
            std::lock_guard<std::mutex> lck(mtx_);
            orders_.erase(ord.id());
          } else {
            std::lock_guard<std::mutex> lck(mtx_);
            auto it = orders_.find(ord.id());
            if (it != orders_.end()) {
              it->second->CopyFrom(ord);
            } else {
              // auto order = Message::NewProto<Proto::Order>();
              // order->CopyFrom(ord);
              orders_.emplace(ord.id(), std::make_shared<Proto::Order>(ord));
            }
          }
        }
      } else if (requests[i]->type() == Proto::RequestType::Del) {}
    }
  }
}

int OrderDB::Callback(void *data, int argc, char **argv, char **col_name) {
  auto *tmp = static_cast<std::tuple<OrderMap*, InstrumentDB*>*>(data);
  auto *orders = std::get<0>(*tmp);
  auto *instrument_db = std::get<1>(*tmp);

  const std::string instrument = argv[1];
  auto inst = instrument_db->FindInstrument(instrument);
  if (inst) {
    auto order = std::make_shared<Proto::Order>();
    char *end = NULL;
    LOG_DBG << "Retrive order id = " << argv[0];
    size_t id = strtoull(argv[0], &end, 10);
    order->set_id(id);
    order->set_instrument(instrument);
    order->set_counter_id(argv[2]);
    order->set_exchange_id(argv[3]);
    order->set_strategy(argv[4]);
    order->set_note(argv[5]);
    order->set_price(atof(argv[6]));
    order->set_avg_executed_price(atof(argv[7]));
    order->set_volume(atoi(argv[8]));
    order->set_executed_volume(atoi(argv[9]));
    order->set_exchange(static_cast<Proto::Exchange>(atoi(argv[10])));
    order->set_strategy_type(static_cast<Proto::StrategyType>(atoi(argv[11])));
    order->set_side(static_cast<Proto::Side>(atoi(argv[12])));
    order->set_time_condition(static_cast<Proto::TimeCondition>(atoi(argv[13])));
    order->set_type(static_cast<Proto::OrderType>(atoi(argv[14])));
    order->set_status(static_cast<Proto::OrderStatus>(atoi(argv[15])));
    order->set_time(base::StringToTime(argv[16]));
    order->set_latency(atoi(argv[17]));
    (*orders)[id] = order;
    LOG_DBG << "Retrive order: " << order->ShortDebugString();
  } else {
    LOG_ERR << "Can't find instrument " << instrument;
  }
  return 0;
}
