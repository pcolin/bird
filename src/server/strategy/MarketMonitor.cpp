#include <sys/time.h>
#include "MarketMonitor.h"
#include "Price.pb.h"
#include "Order.pb.h"
#include "Trade.pb.h"
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"
#include "model/OrderManager.h"
#include "model/PositionManager.h"
#include "model/Middleware.h"
#include "boost/format.hpp"

MarketMonitor::MarketMonitor(const std::string &name, DeviceManager *dm)
    : Strategy(name, dm),
      orders_(capacity_) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  und_price_time_ = tv.tv_sec;
  opt_price_time_ = tv.tv_sec;

  dispatcher_.RegisterCallback<Proto::InstrumentReq>(
      std::bind(&MarketMonitor::OnInstrumentReq, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::PriceReq>(
      std::bind(&MarketMonitor::OnPriceReq, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::Position>(
      std::bind(&MarketMonitor::OnPosition, this, std::placeholders::_1));

  order_thread_ = std::make_unique<std::thread>(std::bind(&MarketMonitor::RunOrder, this));
}

void MarketMonitor::OnStart() {
  LOG_INF << "OnStart";
}

void MarketMonitor::OnStop() {
  LOG_INF << "OnStop";
}

void MarketMonitor::OnPrice(const PricePtr &price) {
  LOG_DBG << price;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int now = tv.tv_sec;
  const int max_interval = 15;
  int interval = now - opt_price_time_;
  if (interval >= max_interval) {
    LOG_ERR << boost::format("%1% : Option price feed timeout for %2%s") %
      dm_->GetUnderlying()->Id() % interval;
    opt_price_time_ = now;
  }
  interval = now - und_price_time_;
  if (interval >= max_interval) {
    LOG_ERR << boost::format("%1% : Underlying price feed timeout for %2%s") %
      dm_->GetUnderlying()->Id() % interval;
    und_price_time_ = now;
  }
  if (price->instrument->Type() == Proto::InstrumentType::Option) {
    opt_price_time_ = now;
  } else {
    und_price_time_ = now;
  }
  auto p = price->Serialize();
  prices_[price->instrument] = p;
  Middleware::GetInstance()->Publish(p);
}

void MarketMonitor::OnOrder(const OrderPtr &order) {
  // LOG_INF << "OnOrder : " << order->DebugString();
  OrderManager::GetInstance()->OnOrder(order);
  orders_.enqueue(order);
  // Middleware::GetInstance()->Publish(order->Serialize());
}

void MarketMonitor::OnTrade(const TradePtr &trade) {
  // LOG_INF << "OnTrade : " << trade->DebugString();
  Middleware::GetInstance()->Publish(trade->Serialize());
}

// void MarketMonitor::OnLastEvent()
// {
//   if (orders_.size() > 0)
//   {
//     LOG_INF << boost::format("Update %1% orders") % orders_.size();
//     OrderManager::GetInstance()->OnOrder(orders_);
//     orders_.clear();
//   }
// }

bool MarketMonitor::OnInstrumentReq(const std::shared_ptr<Proto::InstrumentReq> &req) {
  LOG_INF << "InstrumentReq: " << req->ShortDebugString();
  // Middleware::GetInstance()->Publish(status);
}

bool MarketMonitor::OnPosition(const std::shared_ptr<Proto::Position> &position) {
  LOG_INF << "Position: " << position->ShortDebugString();
  PositionManager::GetInstance()->UpdatePosition(position);
  return true;
}

bool MarketMonitor::OnPriceReq(const std::shared_ptr<Proto::PriceReq> &req) {
  LOG_INF << "PriceReq: " << req->ShortDebugString();
  for (auto it : prices_) {
    Middleware::GetInstance()->Publish(it.second);
  }
  return true;
}

void MarketMonitor::RunOrder() {
  LOG_INF << "Start order publishing thread...";
  OrderPtr orders[capacity_];
  auto req = Message::NewProto<Proto::OrderReq>();
  req->set_type(Proto::RequestType::Set);
  req->set_exchange(Underlying()->Exchange());
  const std::string user = EnvConfig::GetInstance()->GetString(EnvVar::EXCHANGE);
  req->set_user(user);
  std::unordered_map<size_t, int> indexes;
  while (true) {
    size_t cnt = orders_.wait_dequeue_bulk(orders, capacity_);
    for (size_t i = 0; i < cnt; ++i) {
      auto it = indexes.find(orders[i]->id);
      if (it != indexes.end()) {
        if (orders[it->second]->status < orders[i]->status ||
            orders[it->second]->executed_volume < orders[i]->executed_volume) {
          orders[it->second] = orders[i];
        }
        orders[i] = nullptr;
      } else {
        indexes[orders[i]->id] = i;
      }
    }
    for (size_t i = 0; i < cnt; ++i) {
      if (orders[i]) {
        orders[i]->Serialize(req->add_orders());
        if (req->orders_size()  == 10) {
          Middleware::GetInstance()->Publish(req);
          req = Message::NewProto<Proto::OrderReq>();
          req->set_type(Proto::RequestType::Set);
          req->set_exchange(Underlying()->Exchange());
          req->set_user(user);
        }
      }
    }
    if (req->orders_size()) {
      Middleware::GetInstance()->Publish(req);
      req = Message::NewProto<Proto::OrderReq>();
      req->set_type(Proto::RequestType::Set);
      req->set_exchange(Underlying()->Exchange());
      req->set_user(user);
    }
    indexes.clear();
  }
}
