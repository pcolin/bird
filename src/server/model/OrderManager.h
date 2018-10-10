#ifndef MODEL_ORDER_MANAGER_H
#define MODEL_ORDER_MANAGER_H

#include <vector>
#include <mutex>
#include "InactiveOrderManager.h"

class OrderManager {
 public:
  typedef std::unordered_map<size_t, OrderPtr> OrderMap;

  static OrderManager* GetInstance();
  ~OrderManager() {}

  void Init();

  OrderPtr FindOrder(const size_t id);
  OrderPtr FindOrder(const std::string &exchange_id);
  OrderPtr FindActiveOrder(const size_t id);
  OrderPtr FindActiveOrder(const std::string &exchange_id);
  OrderPtr FindActiveCounterOrder(const std::string &counter_id);
  OrderPtr FindInactiveOrder(const size_t id);
  OrderPtr FindInactiveOrder(const std::string &exchange_id);

  void OnOrder(const OrderPtr &order);
  void OnOrder(const std::vector<OrderPtr> &orders);

  void Dump();

 private:
  OrderManager() {}
  void UpdateOrder(const OrderPtr &order);

  OrderMap active_orders_;
  typedef std::unordered_map<std::string, OrderPtr> ExchOrderMap;
  ExchOrderMap exch_active_orders_;
  InactiveOrderManager inactive_manager_;
  std::mutex mtx_;
};

#endif // MODEL_ORDER_MANAGER_H
