#include "InactiveOrderManager.h"
#include "config/EnvConfig.h"
#include "base/logger/Logging.h"
#include <boost/format.hpp>

using namespace base;

InactiveOrderManager::InactiveOrderManager()
  : gc_threshold_(EnvConfig::GetInstance()->GetInt32(EnvVar::ORDER_GC_THRESHOLD, 10000))
{
  generations_[0] = std::make_shared<Generation>();
  generations_[1] = std::make_shared<Generation>();
}

void InactiveOrderManager::Insert(const OrderPtr &order)
{
  assert(generations_.front());
  std::get<0>(*generations_.front())[order->id] = order;
  std::get<1>(*generations_.front())[order->exchange_id] = order;
  if (++order_num_ % gc_threshold_ == 0)
  {
    LOG_INF << boost::format("GC collects %1% inactive order") %
      std::get<0>(*generations_.front()).size();
    for(int i = GENERATION_NUM - 1; i > 0; --i)
    {
      generations_[i] = std::move(generations_[i-1]);
    }
    generations_.front() = std::make_shared<Generation>();
  }
}

void InactiveOrderManager::Remove(const OrderPtr &order)
{
  for (auto &gen : generations_)
  {
    if (gen)
    {
      std::get<0>(*gen).erase(order->id);
      if (!order->exchange_id.empty())
      {
        std::get<1>(*gen).erase(order->exchange_id);
      }
    }
  }
}

OrderPtr InactiveOrderManager::FindOrder(const size_t id) const
{
  for (auto &gen : generations_)
  {
    if (gen)
    {
      auto &orders = std::get<0>(*gen);
      auto it = orders.find(id);
      if (it != orders.end())
      {
        return it->second;
      }
    }
  }
  return nullptr;
}

OrderPtr InactiveOrderManager::FindOrder(const std::string &exchange_id) const
{
  for (auto &gen : generations_)
  {
    if (gen)
    {
      auto &orders = std::get<1>(*gen);
      auto it = orders.find(exchange_id);
      if (it != orders.end())
      {
        return it->second;
      }
    }
  }
  return nullptr;
}

size_t InactiveOrderManager::Size() const
{
  size_t cnt = 0;
  for (auto &gen : generations_)
  {
    if (gen)
    {
      cnt += std::get<0>(*gen).size();
    }
  }
  return cnt;
}

void InactiveOrderManager::Dump() const
{
  size_t cnt = 0;
  for (auto &gen : generations_)
  {
    if (gen)
    {
      auto &orders = std::get<0>(*gen);
      cnt += orders.size();
      for(auto &it : orders)
      {
        LOG_INF << it.second;
      }
    }
  }
  LOG_INF << boost::format("Dump %1% inactive orders...") % cnt;
}
