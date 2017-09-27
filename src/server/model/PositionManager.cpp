#include "PositionManager.h"
#include "ProductManager.h"
#include "base/logger/Logging.h"

#include <boost/format.hpp>

PositionManager* PositionManager::GetInstance()
{
  static PositionManager manager;
  return &manager;
}

void PositionManager::Init()
{
  LOG_INF << "Initialize position manager...";
  auto insts = ProductManager::GetInstance()->FindInstruments([](const Instrument*){ return true; });
  std::lock_guard<std::mutex> lck(mtx_);
  for (auto &inst : insts)
  {
    positions_.emplace(inst, std::make_shared<PROTO::Position>());
  }
}

bool PositionManager::TryFreeze(const OrderPtr &order)
{
  std::lock_guard<std::mutex> lck(mtx_);
  PositionPtr &pos = positions_[order->instrument];
  assert(pos);
  if (order->IsBid())
  {
    if (order->volume <= pos->liquid_short())
    {
      pos->set_liquid_short(pos->liquid_short() - order->volume);
      LOG_INF << boost::format("%1% short pos update: %2%(%3%)/%4%") % order->instrument->Id() %
        pos->liquid_short() % (pos->total_short() - pos->liquid_short()) % pos->total_short();
      return true;
    }
  }
  else
  {
    if (order->volume <= pos->liquid_long())
    {
      pos->set_liquid_long(pos->liquid_long() - order->volume);
      LOG_INF << boost::format("%1% long pos update: %2%(%3%)/%4%") % order->instrument->Id() %
        pos->liquid_long() % (pos->total_long() - pos->liquid_long()) % pos->total_long();
      return true;
    }
  }

  return false;
}

void PositionManager::Release(const OrderPtr &order)
{
  if (!order->IsOpen())
  {
    std::lock_guard<std::mutex> lck(mtx_);
    auto &pos = positions_[order->instrument];
    assert(pos);
    if (order->IsBid())
    {
      pos->set_liquid_short(pos->liquid_short() + order->volume - order->executed_volume);
      LOG_INF << boost::format("%1% short pos update: %2%(%3%)/%4%") % order->instrument->Id() %
        pos->liquid_short() % (pos->total_short() - pos->liquid_short()) % pos->total_short();
    }
    else
    {
      pos->set_liquid_long(pos->liquid_long() + order->volume - order->executed_volume);
      LOG_INF << boost::format("%1% long pos update: %2%(%3%)/%4%") % order->instrument->Id() %
        pos->liquid_long() % (pos->total_long() - pos->liquid_long()) % pos->total_long();
    }
  }
}

void PositionManager::UpdatePosition(const Instrument *inst, const PositionPtr &position)
{
  std::lock_guard<std::mutex> lck(mtx_);
  auto &pos = positions_[inst];
  assert(pos);
  LOG_INF << boost::format("%1% pos update: liquid(%2%/%3%->%4%/%5%), total(%6%/%7%->%8%/%9%), "
      "yesterday(%10%,%11%)") %
    pos->liquid_long() % pos->liquid_short() % position->liquid_long() % position->liquid_short() %
    pos->total_long() % pos->total_short() % position->total_long() % position->total_short() %
    position->yesterday_long() % position->yesterday_short();
  positions_[inst] = position;
}

void PositionManager::OnTrade(const TradePtr &trade)
{
  std::lock_guard<std::mutex> lck(mtx_);
  auto &pos = positions_[trade->instrument];
  assert(pos);
  if (trade->side == Side::Buy)
  {
    pos->set_total_long(pos->total_long() + trade->volume);
    LOG_INF << boost::format("%1% long pos update: %2%(%3%)/%4%") % trade->instrument->Id() %
      pos->liquid_long() % (pos->total_long() - pos->liquid_long()) % pos->total_long();
  }
  else
  {
    pos->set_total_short(pos->total_short() + trade->volume);
    LOG_INF << boost::format("%1% short pos update: %2%(%3%)/%4%") % trade->instrument->Id() %
      pos->liquid_short() % (pos->total_short() - pos->liquid_short()) % pos->total_short();
  }
}