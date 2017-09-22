#include "OrderManager.h"
#include "PositionManager.h"
#include "base/common/Likely.h"
#include "base/logger/Logging.h"
#include <boost/format.hpp>

using namespace base;

OrderManager* OrderManager::GetInstance()
{
  static OrderManager manager;
  return &manager;
}

// OrderManager::~OrderManager()
// {
//   LOG_INF << "~OrderManager";
// }

void OrderManager::Init()
{
  LOG_INF << "Initialize order manager...";
  /// to be done... query orders from db.
}

OrderPtr OrderManager::FindOrder(const size_t id)
{
  auto ord = FindActiveOrder(id);
  return ord ? ord : FindInactiveOrder(id);
}

OrderPtr OrderManager::FindOrder(const std::string &exchange_id)
{
  auto ord = FindActiveOrder(exchange_id);
  return ord ? ord : FindInactiveOrder(exchange_id);
}

OrderPtr OrderManager::FindActiveOrder(const size_t id)
{
  std::lock_guard<std::mutex> lck(mtx_);
  auto it = active_orders_.find(id);
  return it != active_orders_.end() ? it->second : nullptr;
}

OrderPtr OrderManager::FindActiveOrder(const std::string &exchange_id)
{
  std::lock_guard<std::mutex> lck(mtx_);
  auto it = exch_active_orders_.find(exchange_id);
  return it != exch_active_orders_.end() ? it->second : nullptr;
}

OrderPtr OrderManager::FindInactiveOrder(const size_t id)
{
  std::lock_guard<std::mutex> lck(mtx_);
  return inactive_manager_.FindOrder(id);
}

OrderPtr OrderManager::FindInactiveOrder(const std::string &exchange_id)
{
  std::lock_guard<std::mutex> lck(mtx_);
  return inactive_manager_.FindOrder(exchange_id);
}

void OrderManager::OnOrder(const OrderPtr &order)
{
  std::lock_guard<std::mutex> lck(mtx_);
  UpdateOrder(order);
}

void OrderManager::OnOrder(const std::vector<OrderPtr> &orders)
{
  std::lock_guard<std::mutex> lck(mtx_);
  for (auto &ord : orders)
  {
    UpdateOrder(ord);
  }
}

void OrderManager::UpdateOrder(const OrderPtr &order)
{
  if (order->IsInactive())
  {
    auto it = active_orders_.find(order->id);
    if (it != active_orders_.end())
    {
      active_orders_.erase(it);
    }
    if (!order->exchange_id.empty())
    {
      exch_active_orders_.erase(order->exchange_id);
    }
    if (!inactive_manager_.FindOrder(order->id))
    {
      PositionManager::GetInstance()->Release(order);
    }
    inactive_manager_.Insert(order);
  }
  else
  {
    auto inactive_order = inactive_manager_.FindOrder(order->id);
    if (unlikely(!!inactive_order))
    {
      LOG_ERR << boost::format("Order becomes to active from inactive (%1%)") % order->Dump();
      inactive_manager_.Remove(order);
    }
    active_orders_[order->id] = order;
    if (!order->exchange_id.empty())
    {
      exch_active_orders_[order->exchange_id] = order;
    }
  }
}

void OrderManager::Dump()
{
  std::lock_guard<std::mutex> lck(mtx_);
  LOG_INF << boost::format("Dump %1% active orders...") % active_orders_.size();
  for (auto &it : active_orders_)
  {
    LOG_INF << it.second->Dump();
  }
  inactive_manager_.Dump();
}
