#include "PositionManager.h"
#include "ProductManager.h"
#include "Middleware.h"
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
    auto pos = std::make_shared<Proto::Position>();
    pos->set_instrument(inst->Id());
    positions_.emplace(inst, pos);
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
      LOG_INF << boost::format("%1% short pos update: %2%(-%3%)/%4%") % order->instrument->Id() %
        pos->liquid_short() % order->volume % pos->total_short();
      PublishPosition(order->instrument->Exchange(), pos);
      return true;
    }
  }
  else
  {
    if (order->volume <= pos->liquid_long())
    {
      pos->set_liquid_long(pos->liquid_long() - order->volume);
      LOG_INF << boost::format("%1% long pos update: %2%(-%3%)/%4%") % order->instrument->Id() %
        pos->liquid_long() % order->volume % pos->total_long();
      PublishPosition(order->instrument->Exchange(), pos);
      return true;
    }
  }

  return false;
}

void PositionManager::Release(const OrderPtr &order)
{
  if (!order->IsOpen() && order->executed_volume < order->volume)
  {
    std::lock_guard<std::mutex> lck(mtx_);
    auto &pos = positions_[order->instrument];
    assert(pos);
    if (order->IsBid())
    {
      pos->set_liquid_short(pos->liquid_short() + order->volume - order->executed_volume);
      LOG_INF << boost::format("%1% short pos update: %2%(+%3%)/%4%") % order->instrument->Id() %
        pos->liquid_short() % (order->volume - order->executed_volume) % pos->total_short();
    }
    else
    {
      pos->set_liquid_long(pos->liquid_long() + order->volume - order->executed_volume);
      LOG_INF << boost::format("%1% long pos update: %2%(+%3%)/%4%") % order->instrument->Id() %
        pos->liquid_long() % (order->volume - order->executed_volume) % pos->total_long();
    }
    PublishPosition(order->instrument->Exchange(), pos);
  }
}

void PositionManager::UpdatePosition(const PositionPtr &position)
{
  const Instrument *inst = ProductManager::GetInstance()->FindId(position->instrument());
  assert(inst);
  std::lock_guard<std::mutex> lck(mtx_);
  auto &pos = positions_[inst];
  assert(pos);
  LOG_INF << boost::format("%1% pos update: liquid(%2%/%3%->%4%/%5%), total(%6%/%7%->%8%/%9%), "
      "yesterday(%10%,%11%)") %
    position->instrument() % pos->liquid_long() % pos->liquid_short() % position->liquid_long() %
    position->liquid_short() % pos->total_long() % pos->total_short() % position->total_long() %
    position->total_short() % position->yesterday_long() % position->yesterday_short();
  positions_[inst] = position;
  PublishPosition(inst->Exchange(), positions_[inst]);
}

void PositionManager::OnTrade(const TradePtr &trade)
{
  std::lock_guard<std::mutex> lck(mtx_);
  auto &pos = positions_[trade->instrument];
  assert(pos);
  if (trade->side == Proto::Side::Buy)
  {
    int liquid_long = pos->liquid_long(), total_long = pos->total_long();
    pos->set_liquid_long(liquid_long + trade->volume);
    pos->set_total_long(total_long + trade->volume);
    LOG_INF << boost::format("%1% long pos update: %2%/%3%->%4%/%5%") % trade->instrument->Id() %
      liquid_long % total_long % pos->liquid_long() % pos->total_long();
  }
  else if (trade->side == Proto::Side::Sell)
  {
    int liquid_short = pos->liquid_short(), total_short = pos->total_short();
    pos->set_liquid_short(liquid_short + trade->volume);
    pos->set_total_short(total_short + trade->volume);
    LOG_INF << boost::format("%1% short pos update: %2%/%3%->%4%/%5%") % trade->instrument->Id() %
      liquid_short % total_short % pos->liquid_short() % pos->total_short();
  }
  else if (trade->side == Proto::Side::BuyCover || trade->side == Proto::Side::BuyCoverToday ||
      trade->side == Proto::Side::BuyCoverYesterday)
  {
    int total_short = pos->total_short();
    pos->set_total_short(total_short - trade->volume);
    LOG_INF << boost::format("%1% short pos update: %2%/%3%->%2%/%4%") % trade->instrument->Id() %
      pos->liquid_short() % total_short % pos->total_short();
  }
  else if (trade->side == Proto::Side::SellCover || trade->side == Proto::Side::SellCoverToday ||
      trade->side == Proto::Side::SellCoverYesterday)
  {
    int total_long = pos->total_long();
    pos->set_total_long(total_long - trade->volume);
    LOG_INF << boost::format("%1% long pos update: %2%/%3%->%2%/%4%") % trade->instrument->Id() %
      pos->liquid_long() % total_long % pos->total_long();
  }
  PublishPosition(trade->instrument->Exchange(), pos);
}

void PositionManager::PublishPosition(Proto::Exchange exchange, PositionPtr &position)
{
  position->set_time(base::Now());
  auto req = Message::NewProto<Proto::PositionReq>();
  req->set_type(Proto::RequestType::Set);
  req->set_exchange(exchange);
  req->add_positions()->CopyFrom(*position);
  Middleware::GetInstance()->Publish(req);
}
