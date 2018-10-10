#include "OrderManager.h"
#include "PositionManager.h"
#include "base/common/Likely.h"
#include "base/logger/Logging.h"
#include "boost/format.hpp"

using namespace base;

OrderManager* OrderManager::GetInstance() {
  static OrderManager manager;
  return &manager;
}

// OrderManager::~OrderManager() {
//   LOG_INF << "~OrderManager";
// }

void OrderManager::Init() {
  LOG_INF << "Initialize order manager...";
  /// to be done... query orders from db.
}

OrderPtr OrderManager::FindOrder(const size_t id) {
  auto ord = FindActiveOrder(id);
  return ord ? ord : FindInactiveOrder(id);
}

OrderPtr OrderManager::FindOrder(const std::string &exchange_id) {
  auto ord = FindActiveOrder(exchange_id);
  return ord ? ord : FindInactiveOrder(exchange_id);
}

OrderPtr OrderManager::FindActiveOrder(const size_t id) {
  std::lock_guard<std::mutex> lck(mtx_);
  auto it = active_orders_.find(id);
  return it != active_orders_.end() ? it->second : nullptr;
}

OrderPtr OrderManager::FindActiveOrder(const std::string &exchange_id) {
  std::lock_guard<std::mutex> lck(mtx_);
  auto it = exch_active_orders_.find(exchange_id);
  return it != exch_active_orders_.end() ? it->second : nullptr;
}

OrderPtr OrderManager::FindActiveCounterOrder(const std::string &counter_id) {
  std::lock_guard<std::mutex> lck(mtx_);
  auto it = std::find_if(active_orders_.begin(), active_orders_.end(),
                         [&](auto &vt) { return vt.second->counter_id == counter_id; });
  return it != active_orders_.end() ? it->second : nullptr;
}

OrderPtr OrderManager::FindInactiveOrder(const size_t id) {
  std::lock_guard<std::mutex> lck(mtx_);
  return inactive_manager_.FindOrder(id);
}

OrderPtr OrderManager::FindInactiveOrder(const std::string &exchange_id) {
  std::lock_guard<std::mutex> lck(mtx_);
  return inactive_manager_.FindOrder(exchange_id);
}

void OrderManager::OnOrder(const OrderPtr &order) {
  std::lock_guard<std::mutex> lck(mtx_);
  UpdateOrder(order);
}

void OrderManager::OnOrder(const std::vector<OrderPtr> &orders) {
  std::lock_guard<std::mutex> lck(mtx_);
  for (auto &ord : orders) {
    UpdateOrder(ord);
  }
}

void OrderManager::UpdateOrder(const OrderPtr &order) {
  const size_t act_ord_cnt = active_orders_.size();
  const size_t exch_act_ord_cnt = exch_active_orders_.size();
  const size_t inact_ord_cnt = inactive_manager_.Size();
  if (order->IsInactive()) {
    auto it = active_orders_.find(order->id);
    if (it != active_orders_.end()) {
      active_orders_.erase(it);
    }
    if (!order->exchange_id.empty()) {
      exch_active_orders_.erase(order->exchange_id);
    }
    if (!inactive_manager_.FindOrder(order->id)) {
      PositionManager::GetInstance()->Release(order);
    }
    inactive_manager_.Insert(order);
  } else {
    auto inactive_order = inactive_manager_.FindOrder(order->id);
    if (unlikely(!!inactive_order)) {
      LOG_ERR << "Order becomes to active from inactive (" << order << ')';
      inactive_manager_.Remove(order);
    }
    active_orders_[order->id] = order;
    if (!order->exchange_id.empty()) {
      exch_active_orders_[order->exchange_id] = order;
    }
  }
  LOG_DBG << boost::format("Order books change(%1%, %2%): active(%3%->%4%, %5%->%6%), inactive(%7%"
                           "->%8%)")
             % order->id % order->exchange_id % act_ord_cnt % active_orders_.size()
             % exch_act_ord_cnt % exch_active_orders_.size() % inact_ord_cnt
             % inactive_manager_.Size();
}

void OrderManager::Dump() {
  std::lock_guard<std::mutex> lck(mtx_);
  LOG_INF << boost::format("Dump %1% active orders...") % active_orders_.size();
  for (auto &it : active_orders_) {
    LOG_INF << it.second;
  }
  inactive_manager_.Dump();
}
