#include "PositionManager.h"
#include "boost/format.hpp"
#include "InstrumentManager.h"
#include "Middleware.h"
#include "config/EnvConfig.h"
#include "base/logger/Logging.h"

PositionManager* PositionManager::GetInstance() {
  static PositionManager manager;
  return &manager;
}

void PositionManager::Init() {
  LOG_INF << "Initialize position manager...";
  auto insts = InstrumentManager::GetInstance()->FindInstruments(nullptr);
  std::lock_guard<std::mutex> lck(mtx_);
  for (auto &inst : insts) {
    auto pos = std::make_shared<Proto::Position>();
    pos->set_instrument(inst->Id());
    positions_.emplace(inst, pos);
  }
}

int PositionManager::GetNetPosition(const Instrument *inst) {
  std::lock_guard<std::mutex> lck(mtx_);
  auto it = positions_.find(inst);
  if (it != positions_.end()) {
    return it->second->total_long() - it->second->total_short();
  }
  return 0;
}

bool PositionManager::TryFreeze(const OrderPtr &order) {
  std::lock_guard<std::mutex> lck(mtx_);
  PositionPtr &pos = positions_[order->instrument];
  assert(pos);
  static bool close_today = EnvConfig::GetInstance()
                            ->GetBool(EnvVar::CLOSE_TODAY_POSITION);
  if (order->IsBid()) {
    if (close_today) {
      if (order->volume <= pos->liquid_yesterday_short()) {
        pos->set_liquid_short(pos->liquid_short() - order->volume);
        pos->set_liquid_yesterday_short(pos->liquid_yesterday_short() - order->volume);
        LOG_INF << boost::format("%1% yesterday short pos update: %2%(-%3%)/%4%/%5%") %
                   order->instrument->Id() %
                   pos->liquid_yesterday_short() %
                   order->volume %
                   (pos->liquid_short() - pos->liquid_yesterday_short()) %
                   pos->total_short();
        PublishPosition(pos);
        order->side = Proto::Side::BuyCoverYesterday;
        return true;
      } else if (order->volume <= pos->liquid_short() - pos->liquid_yesterday_short()) {
        pos->set_liquid_short(pos->liquid_short() - order->volume);
        LOG_INF << boost::format("%1% today short pos update: %2%/%3%(-%4%)/%5%") %
                   order->instrument->Id() %
                   pos->liquid_yesterday_short() %
                   (pos->liquid_short() - pos->liquid_yesterday_short()) %
                   order->volume %
                   pos->total_short();
        PublishPosition(pos);
        order->side = Proto::Side::BuyCoverToday;
        return true;
      }
    } else if (order->volume <= pos->liquid_short()) {
      pos->set_liquid_short(pos->liquid_short() - order->volume);
      LOG_INF << boost::format("%1% short pos update: %2%(-%3%)/%4%") %
                 order->instrument->Id() %
                 pos->liquid_short() %
                 order->volume %
                 pos->total_short();
      PublishPosition(pos);
      order->side = Proto::Side::BuyCover;
      return true;
    }
  } else {
    if (close_today) {
      if (order->volume <= pos->liquid_yesterday_long()) {
        pos->set_liquid_long(pos->liquid_long() - order->volume);
        pos->set_liquid_yesterday_long(pos->liquid_yesterday_long() - order->volume);
        LOG_INF << boost::format("%1% yesterday long pos update: %2%(-%3%)/%4%/%5%") %
                   order->instrument->Id() %
                   pos->liquid_yesterday_long() %
                   order->volume %
                   (pos->liquid_long() - pos->liquid_yesterday_long()) %
                   pos->total_long();
        PublishPosition(pos);
        order->side == Proto::Side::SellCoverYesterday;
        return true;
      } else if (order->volume <= pos->liquid_long() - pos->liquid_yesterday_long()) {
        pos->set_liquid_long(pos->liquid_long() - order->volume);
        LOG_INF << boost::format("%1% today long pos update: %2%/%3%(-%4%)/%5%") %
                   order->instrument->Id() %
                   pos->liquid_yesterday_long() %
                   (pos->liquid_long() - pos->liquid_yesterday_long()) %
                   order->volume %
                   pos->total_long();
        PublishPosition(pos);
        order->side == Proto::Side::SellCoverToday;
        return true;
      }
    } else if (order->volume <= pos->liquid_long()) {
      pos->set_liquid_long(pos->liquid_long() - order->volume);
      LOG_INF << boost::format("%1% long pos update: %2%(-%3%)/%4%") %
                 order->instrument->Id() %
                 pos->liquid_long() %
                 order->volume %
                 pos->total_long();
      PublishPosition(pos);
      order->side == Proto::Side::SellCover;
      return true;
    }
  }
  return false;
}

void PositionManager::Release(const OrderPtr &order) {
  if (!order->IsOpen() && order->executed_volume < order->volume) {
    std::lock_guard<std::mutex> lck(mtx_);
    auto &pos = positions_[order->instrument];
    assert(pos);
    switch (order->side) {
      case Proto::Side::BuyCover: {
        pos->set_liquid_short(pos->liquid_short() + order->volume - order->executed_volume);
        LOG_INF << boost::format("%1% short pos update: %2%(+%3%)/%4%") % order->instrument->Id() %
          pos->liquid_short() % (order->volume - order->executed_volume) % pos->total_short();
      break;
      }
      case Proto::Side::BuyCoverYesterday: {
        int volume = order->volume - order->executed_volume;
        pos->set_liquid_short(pos->liquid_short() + volume);
        pos->set_liquid_yesterday_short(pos->liquid_yesterday_short() + volume);
        LOG_INF << boost::format("%1% yesterday short pos update: %2%(+%3%)/%4%/%5%") %
                   order->instrument->Id() %
                   pos->liquid_yesterday_short() %
                   volume %
                   (pos->liquid_short() - pos->liquid_yesterday_short()) %
                   pos->total_short();
      break;
      }
      case Proto::Side::BuyCoverToday: {
        int volume = order->volume - order->executed_volume;
        pos->set_liquid_short(pos->liquid_short() + volume);
        LOG_INF << boost::format("%1% today short pos update: %2%/%3%(+%4%)/%5%") %
                   order->instrument->Id() %
                   pos->liquid_yesterday_short() %
                   (pos->liquid_short() - pos->liquid_yesterday_short()) %
                   volume %
                   pos->total_short();
      break;
      }
      case Proto::Side::SellCover: {
        pos->set_liquid_long(pos->liquid_long() + order->volume - order->executed_volume);
        LOG_INF << boost::format("%1% long pos update: %2%(+%3%)/%4%") %
                   order->instrument->Id() %
                   pos->liquid_long() %
                   (order->volume - order->executed_volume) %
                   pos->total_long();
      break;
      }
      case Proto::Side::SellCoverYesterday: {
        int volume = order->volume - order->executed_volume;
        pos->set_liquid_short(pos->liquid_long() + volume);
        pos->set_liquid_yesterday_long(pos->liquid_yesterday_long() + volume);
        LOG_INF << boost::format("%1% yesterday long pos update: %2%(+%3%)/%4%/%5%") %
                   order->instrument->Id() %
                   pos->liquid_yesterday_long() %
                   volume %
                   (pos->liquid_long() - pos->liquid_yesterday_long()) %
                   pos->total_long();
      break;
      }
      case Proto::Side::SellCoverToday: {
        int volume = order->volume - order->executed_volume;
        pos->set_liquid_long(pos->liquid_long() + volume);
        LOG_INF << boost::format("%1% today long pos update: %2%/%3%(+%4%)/%5%") %
                   order->instrument->Id() %
                   pos->liquid_yesterday_long() %
                   (pos->liquid_long() - pos->liquid_yesterday_long()) %
                   volume %
                   pos->total_long();
      break;
      }
      default:
        assert(false);
    }
    PublishPosition(pos);
  }
}

void PositionManager::UpdatePosition(const PositionPtr &position) {
  const Instrument *inst = InstrumentManager::GetInstance()->FindId(position->instrument());
  assert(inst);
  std::lock_guard<std::mutex> lck(mtx_);
  auto &pos = positions_[inst];
  assert(pos);
  LOG_INF << boost::format("%1% pos update: liquid(%2%/%3%->%4%/%5%), total(%6%/%7%->%8%/%9%), "
                           "yesterday(%10%,%11%)") %
             position->instrument() %
             pos->liquid_long() %
             pos->liquid_short() %
             position->liquid_long() %
             position->liquid_short() %
             pos->total_long() %
             pos->total_short() %
             position->total_long() %
             position->total_short() %
             position->yesterday_long() %
             position->yesterday_short();
  positions_[inst] = position;
  PublishPosition(positions_[inst]);
}

void PositionManager::OnTrade(const TradePtr &trade) {
  std::lock_guard<std::mutex> lck(mtx_);
  auto &pos = positions_[trade->instrument];
  assert(pos);
  switch (trade->side) {
    case Proto::Side::Buy: {
      int liquid_long = pos->liquid_long(), total_long = pos->total_long();
      pos->set_liquid_long(liquid_long + trade->volume);
      pos->set_total_long(total_long + trade->volume);
      LOG_INF << boost::format("%1% long pos update: %2%/%3%->%4%/%5%") %
                 trade->instrument->Id() %
                 liquid_long %
                 total_long %
                 pos->liquid_long() %
                 pos->total_long();
    break;
    }
    case Proto::Side::Sell: {
      int liquid_short = pos->liquid_short(), total_short = pos->total_short();
      pos->set_liquid_short(liquid_short + trade->volume);
      pos->set_total_short(total_short + trade->volume);
      LOG_INF << boost::format("%1% short pos update: %2%/%3%->%4%/%5%") %
                 trade->instrument->Id() %
                 liquid_short %
                 total_short %
                 pos->liquid_short() %
                 pos->total_short();
    break;
    }
    case Proto::Side::BuyCover: {
      int total_short = pos->total_short();
      pos->set_total_short(total_short - trade->volume);
      LOG_INF << boost::format("%1% short pos update: %2%/%3%->%2%/%4%") %
                 trade->instrument->Id() %
                 pos->liquid_short() %
                 total_short %
                 pos->total_short();
    break;
    }
    case Proto::Side::BuyCoverToday: {
      int total_short = pos->total_short();
      pos->set_total_short(total_short - trade->volume);
      LOG_INF << boost::format("%1% today short pos update: %2%/%3%->%2%/%4%") %
                 trade->instrument->Id() %
                 pos->liquid_short() %
                 total_short %
                 pos->total_short();
    break;
    }
    case Proto::Side::BuyCoverYesterday: {
      int total_short = pos->total_short();
      pos->set_total_short(total_short - trade->volume);
      LOG_INF << boost::format("%1% yesterday short pos update: %2%/%3%->%2%/%4%") %
                 trade->instrument->Id() %
                 pos->liquid_short() %
                 total_short %
                 pos->total_short();
    break;
    }
    case Proto::Side::SellCover: {
      int total_long = pos->total_long();
      pos->set_total_long(total_long - trade->volume);
      LOG_INF << boost::format("%1% long pos update: %2%/%3%->%2%/%4%") %
                 trade->instrument->Id() %
                 pos->liquid_long() %
                 total_long %
                 pos->total_long();
    break;
    }
    case Proto::Side::SellCoverToday: {
      int total_long = pos->total_long();
      pos->set_total_long(total_long - trade->volume);
      LOG_INF << boost::format("%1% today long pos update: %2%/%3%->%2%/%4%") %
                 trade->instrument->Id() %
                 pos->liquid_long() %
                 total_long %
                 pos->total_long();
    break;
    }
    case Proto::Side::SellCoverYesterday: {
      int total_long = pos->total_long();
      pos->set_total_long(total_long - trade->volume);
      LOG_INF << boost::format("%1% yesterday long pos update: %2%/%3%->%2%/%4%") %
                 trade->instrument->Id() %
                 pos->liquid_long() %
                 total_long %
                 pos->total_long();
    break;
    }
    default: {
      assert(false);
    }
  }
  // PublishPosition(trade->instrument->Exchange(), pos);
  PublishPosition(pos);
}

void PositionManager::PublishPosition(PositionPtr &position) {
  position->set_time(base::Now());
  // auto req = Message<Proto::PositionReq>::New();
  // req->set_type(Proto::RequestType::Set);
  // req->set_exchange(exchange);
  // req->add_positions()->CopyFrom(*position);
  Middleware::GetInstance()->Publish(Message<Proto::Position>::New(position));
}
