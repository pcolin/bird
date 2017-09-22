#ifndef MODEL_ORDER_MANAGER_H
#define MODEL_ORDER_MANAGER_H

#include "InactiveOrderManager.h"
#include <vector>
#include <mutex>

class OrderManager
{
public:
  static OrderManager* GetInstance();
  ~OrderManager() {}

  void Init();

  OrderPtr FindOrder(const size_t id);
  OrderPtr FindOrder(const std::string &exchange_id);
  OrderPtr FindActiveOrder(const size_t id);
  OrderPtr FindActiveOrder(const std::string &exchange_id);
  OrderPtr FindInactiveOrder(const size_t id);
  OrderPtr FindInactiveOrder(const std::string &exchange_id);

  void OnOrder(const OrderPtr &order);
  void OnOrder(const std::vector<OrderPtr> &orders);

  void Dump();

private:
  OrderManager() {}
  void UpdateOrder(const OrderPtr &order);

  typedef std::unordered_map<size_t, OrderPtr> OrderMap;
  OrderMap active_orders_;
  typedef std::unordered_map<std::string, OrderPtr> ExchOrderMap;
  ExchOrderMap exch_active_orders_;
  InactiveOrderManager inactive_manager_;
  std::mutex mtx_;
};

#endif
