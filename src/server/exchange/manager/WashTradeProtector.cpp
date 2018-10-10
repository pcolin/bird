#include "WashTradeProtector.h"
#include "base/common/Likely.h"
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"
#include "boost/format.hpp"

using namespace base;

WashTradeProtector::WashTradeProtector()
    : enabled_(EnvConfig::GetInstance()->GetBool(EnvVar::WASH_TRADE_PROT, true)) {}

bool WashTradeProtector::TryAdd(const OrderPtr& order) {
  if (unlikely(!enabled_ || !order)) {
    return true;
  }

  std::unique_lock<std::mutex> lck(mtx_);
  auto &bids = std::get<0>(orders_[order->instrument]);
  auto &asks = std::get<1>(orders_[order->instrument]);
  if (order->IsBid()) {
    auto it = std::find_if(asks.begin(), asks.end(),
                           [&](const OrderPtr &ord) { return order->price >= ord->price; });
    if (it == asks.end()) {
      bids.push_back(order);
      return true;
    } else {
      // LOG_INF << boost::format("Wash trade protection : %1%") % order->price;
      return false;
    }
  } else {
    auto it = std::find_if(bids.begin(), bids.end(),
                           [&](const OrderPtr &ord) { return order->price <= ord->price; });
    if (it == bids.end()) {
      asks.push_back(order);
      return true;
    } else {
      // LOG_INF << boost::format("Wash trade protection : ");
      return false;
    }
  }
}

bool WashTradeProtector::TryAdd(const OrderPtr& bid, const OrderPtr& ask) {
  if (unlikely(!enabled_)) {
    return true;
  }

  if (unlikely(bid && ask && bid->price >= ask->price)) {
    return false;
  }

  std::unique_lock<std::mutex> lck(mtx_);
  auto& bids = std::get<0>(orders_[bid->instrument]);
  auto& asks = std::get<1>(orders_[bid->instrument]);
  if (bid) {
    auto it = std::find_if(asks.begin(), asks.end(),
                           [&](const OrderPtr &ord) { return bid->price >= ord->price; });
    if (it != asks.end()) {
      // LOG_INF << boost::format("Wash trade protection : %1%") % order->price;
      return false;
    }
  }
  if (ask) {
    auto it = std::find_if(bids.begin(), bids.end(),
                           [&](const OrderPtr &ord) { return ask->price <= ord->price; });
    if (it != bids.end()) {
      // LOG_INF << boost::format("Wash trade protection : ");
      return false;
    }
  }

  if (bid) {
    bids.push_back(bid);
  }
  if (ask) {
    asks.push_back(ask);
  }
  return true;
}

void WashTradeProtector::Remove(const OrderPtr& order) {
  if (unlikely(!enabled_ || !order)) {
    return;
  }

  std::unique_lock<std::mutex> lck(mtx_);
  auto& orders = orders_[order->instrument];
  auto& side_orders = order->IsBid() ? std::get<0>(orders) : std::get<1>(orders);
  side_orders.remove_if([&](const OrderPtr& ord) { return order->id == ord->id; });
}

void WashTradeProtector::Remove(const OrderPtr& bid, const OrderPtr& ask) {
  if (unlikely(!enabled_)) {
    return;
  }

  std::unique_lock<std::mutex> lck(mtx_);
  if (bid) {
    auto& bids = std::get<0>(orders_[bid->instrument]);
    bids.remove_if([&](const OrderPtr &ord) { return bid->id == ord->id; });
  }
  if (ask) {
    auto& asks = std::get<1>(orders_[bid->instrument]);
    asks.remove_if([&](const OrderPtr &ord) { return ask->id == ord->id; });
  }
}
