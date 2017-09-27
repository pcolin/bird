#ifndef MODEL_INACTIVE_ORDER_MANAGER_H
#define MODEL_INACTIVE_ORDER_MANAGER_H

#include "Order.h"
#include <unordered_map>

class InactiveOrderManager
{
public:
  InactiveOrderManager();
  ~InactiveOrderManager() {}

  void Insert(const OrderPtr &order);
  void Remove(const OrderPtr &order);
  OrderPtr FindOrder(const size_t id) const;
  OrderPtr FindOrder(const std::string &exchange_id) const;

  size_t Size() const;
  void Dump() const;

private:
  typedef std::unordered_map<size_t, OrderPtr> OrderMap;
  typedef std::unordered_map<std::string, OrderPtr> ExchOrderMap;
  typedef std::tuple<OrderMap, ExchOrderMap> Generation;
  typedef std::shared_ptr<Generation> GenerationPtr;

  static const int GENERATION_NUM = 3;
  const int gc_threshold_;
  std::array<GenerationPtr, GENERATION_NUM> generations_;
  size_t order_num_ = 0;
};

#endif
