#include "Hitter.h"
#include "strategy/base/ClusterManager.h"
#include "exchange/manager/ExchangeManager.h"
#include "base/logger/Logging.h"
#include "base/common/Float.h"
#include "config/EnvConfig.h"
#include "model/Middleware.h"
#include "model/PositionManager.h"
#include "model/InstrumentManager.h"
#include "boost/format.hpp"

Hitter::Hitter(const std::string &name, DeviceManager *dm)
    : Strategy(name, dm),
      max_volume_(EnvConfig::GetInstance()->GetInt32(EnvVar::MAX_ORDER_SIZE)),
      bid_(Message<Order>::New()), ask_(Message<Order>::New()),
      statistic_(Message<Proto::StrategyStatistic>::New()) {
  dispatcher_.RegisterCallback<Proto::PriceException>(
      std::bind(&Hitter::OnPriceException, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::Credit>(
      std::bind(&Hitter::OnCredit, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::StrategySwitch>(
      std::bind(&Hitter::OnStrategySwitch, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::StrategyOperate>(
      std::bind(&Hitter::OnStrategyOperate, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::InstrumentReq>(
      std::bind(&Hitter::OnInstrumentReq, this, std::placeholders::_1));

  bid_->strategy = name_;
  bid_->side = Proto::Side::Buy;
  bid_->time_condition = Proto::TimeCondition::IOC;
  bid_->type = Proto::OrderType::Limit;
  bid_->status = Proto::OrderStatus::Local;
  ask_->strategy = name_;
  ask_->side = Proto::Side::Sell;
  ask_->time_condition = Proto::TimeCondition::IOC;
  ask_->type = Proto::OrderType::Limit;
  ask_->status = Proto::OrderStatus::Local;

  statistic_->set_name(name_);
  statistic_->set_type(Proto::StrategyType::Hitter);
  auto *underlying = Underlying();
  statistic_->set_exchange(underlying->Exchange());
  statistic_->set_underlying(underlying->Id());
  statistic_->set_status(Proto::StrategyStatus::Running);
}

void Hitter::OnStart() {
  api_ = ExchangeManager::GetInstance()->GetTraderApi();
  assert(api_);

  hitter_ = ClusterManager::GetInstance()->FindHitter(name_);
  if (hitter_) {
    parameters_.clear();
    std::unordered_map<std::string, std::tuple<double, double>> credits;
    auto tmp = ClusterManager::GetInstance()->FindCredits(
        Proto::StrategyType::Hitter, Underlying());
    for (auto &credit : tmp) {
      for (auto &r : credit->records()) {
        credits[r.option()] = std::make_tuple(r.credit(), credit->multiplier());
      }
    }
    LOG_DBG << boost::format("Get %1% credits") % credits.size();
    for (auto &op : hitter_->options()) {
      auto *inst = InstrumentManager::GetInstance()->FindId(op);
      if (inst) {
        auto it = parameters_.find(inst);
        if (it == parameters_.end()) {
          auto p = std::make_shared<Parameter>();
          auto itr = credits.find(op);
          if (itr != credits.end()) {
            p->credit = std::get<0>(itr->second);
            p->multiplier = std::get<1>(itr->second);
          }
          auto sw = ClusterManager::GetInstance()->FindStrategySwitch(
              Proto::StrategyType::Hitter, inst);
          if (sw) {
            p->bid_on = sw->is_bid();
            p->ask_on = sw->is_ask();
            p->is_cover = sw->is_qr_cover();
          }
          parameters_.emplace(inst, p);
        } else {
          LOG_WAN << "duplicate option " << op << " in " << name_;
        }
      }
    }

    orders_.clear();
    statistic_->clear_delta();
    statistic_->clear_orders();
    statistic_->clear_trades();
    statistic_->clear_bid_refills();
    statistic_->clear_ask_refills();
  } else {
    LOG_ERR << "hitter " << name_ << " isn't existed";
    stop_(name_ + "isn't existed");
  }
}

void Hitter::OnStop() {
  LOG_INF << "OnStop";
  Middleware::GetInstance()->Publish(Message<Proto::StrategyStatistic>::New(*statistic_));
}

void Hitter::OnPrice(const PricePtr &price) {
  LOG_DBG << price;
  if (price->instrument->Type() == Proto::InstrumentType::Option) {
    auto it = parameters_.find(price->instrument);
    if (it != parameters_.end()) {
      it->second->bid = price->bids[0];
      it->second->ask = price->asks[0];
      it->second->last = price->last;
      Evaluate(it, Proto::HitType::Active);
    }
  } else if (price->instrument == Underlying()) {
    if (!underlying_price_ || !underlying_price_->bids[0] || !underlying_price_->asks[0] ||
        !base::IsEqual(underlying_price_->bids[0].price, price->bids[0].price) ||
        !base::IsEqual(underlying_price_->asks[0].price, price->asks[0].price)) {
      for (auto it = parameters_.begin(); it != parameters_.end(); ++it) {
        Evaluate(it, Proto::HitType::Passive);
      }
      underlying_price_ = price;
    }
  }
}

void Hitter::OnTheoMatrix(const TheoMatrixPtr &theo) {
  LOG_TRA << theo;
  auto it = parameters_.find(theo->option);
  if (it != parameters_.end()) {
    it->second->theos = theo;
    Evaluate(it, Proto::HitType::Refresh);
  }
}

void Hitter::OnOrder(const OrderPtr &order) {
  LOG_DBG << order;
  auto it = parameters_.find(order->instrument);
  if (unlikely(it == parameters_.end())) {
    return;
  }
  if (order->strategy == name_) {
    auto &orders = order->IsBid() ? it->second->bids : it->second->asks;
    auto itr = orders.find(order->id);
    if (itr != orders.end()) {
      if (order->IsInactive()) {
        orders.erase(itr);
        if (order->status == Proto::OrderStatus::Canceled) {
          orders_.erase(order->id);
        }
      }
      LOG_INF << "order update: " << order;
    }
  } else if(order->status == Proto::OrderStatus::Filled) {
    it->second->last = { order->price, order->volume };
    EvaluateLast(it);
  }
}

void Hitter::OnTrade(const TradePtr &trade) {
  LOG_DBG << trade;
  auto it = orders_.find(trade->order_id);
  if (it != orders_.end()) {
    statistic_->set_trades(statistic_->trades() + 1);
    if (statistic_->trades() >= hitter_->trade_limit()) {
      stop_("trade limit broken");
      return;
    }
    statistic_->set_delta(statistic_->delta() +
        it->second->delta * trade->volume * trade->instrument->Multiplier() /
        trade->instrument->Underlying()->Multiplier());
    if (base::IsMoreThan(statistic_->delta(), hitter_->delta_limit())) {
      stop_("delta limit broken");
      return;
    }
  }
}

bool Hitter::OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat) {
  Middleware::GetInstance()->Publish(Message<Proto::StrategyStatistic>::New(*statistic_));
}

bool Hitter::OnPriceException(const std::shared_ptr<Proto::PriceException> &msg) {
  LOG_INF << msg->ShortDebugString();
  exception_ = msg;
}

bool Hitter::OnCredit(const std::shared_ptr<Proto::Credit> &msg) {
  LOG_DBG << msg->ShortDebugString();
  if (msg->strategy() == Proto::StrategyType::Hitter) {
    for (auto &r : msg->records()) {
      auto *inst = InstrumentManager::GetInstance()->FindId(r.option());
      if (inst) {
        auto it = parameters_.find(inst);
        if (it != parameters_.end()) {
          it->second->multiplier = msg->multiplier();
          it->second->credit = r.credit();
          Evaluate(it, Proto::HitType::Refresh);
        }
      }
    }
  }
}

bool Hitter::OnStrategySwitch(const std::shared_ptr<Proto::StrategySwitch> &msg) {
  LOG_DBG << msg->ShortDebugString();
  if (msg->strategy() == Proto::StrategyType::Hitter) {
    auto *inst = InstrumentManager::GetInstance()->FindId(msg->option());
    if (inst) {
      auto it = parameters_.find(inst);
      if (it != parameters_.end()) {
        it->second->bid_on = msg->is_bid();
        it->second->ask_on = msg->is_ask();
        it->second->is_cover = msg->is_qr_cover();
        Evaluate(it, Proto::HitType::Refresh);
      }
    }
  }
}

bool Hitter::OnStrategyOperate(const std::shared_ptr<Proto::StrategyOperate> &msg) {
  if (msg->name() == Name() && msg->operate() == Proto::StrategyOperation::Start) {
    statistic_->clear_delta();
    statistic_->clear_orders();
    statistic_->clear_trades();
    statistic_->clear_bid_refills();
    statistic_->clear_ask_refills();
    for (auto it = parameters_.begin(); it != parameters_.end(); ++it) {
      it->second->bid_refills = it->second->ask_refills = 0;
      Evaluate(it, Proto::HitType::Play);
    }
  }
}

bool Hitter::OnInstrumentReq(const std::shared_ptr<Proto::InstrumentReq> &msg) {
  LOG_INF << "InstrumentReq: " << msg->ShortDebugString();
  for (auto &inst : msg->instruments()) {
    auto *instrument = InstrumentManager::GetInstance()->FindId(inst.id());
    if (instrument && instrument->HedgeUnderlying () == Underlying()) {
      if (inst.type() == Proto::InstrumentType::Option) {
        auto it = parameters_.find(instrument);
        if (it != parameters_.end()) {
          Evaluate(it, Proto::HitType::Active);
        }
      } else {
        for (auto it = parameters_.begin(); it != parameters_.end(); ++it) {
          Evaluate(it, Proto::HitType::Passive);
        }
      }
    }
  }
}

void Hitter::EvaluateLast(ParameterMap::iterator &it) {
  if (it->second->theos && it->first->Status() == Proto::InstrumentStatus::Trading) {
    double spot = dm_->GetUnderlyingTheo();
    TheoData theo;
    if (it->second->theos->FindTheo(spot, theo)) {
      auto credit = it->second->credit * 2;
      if (exception_ && exception_->on()) {
        credit *= (it->second->multiplier * exception_->multiplier());
      }
      if (it->second->ask_on && it->second->bid && it->second->bids.empty() &&
          it->second->last.price >= theo.theo + credit &&
          it->second->ask_refills < hitter_->refill_limit()) {
        if (unlikely(statistic_->orders() >= hitter_->order_limit())) {
          stop_("order limit broken");
          return;
        }
        auto size = base::IsEqual(credit, 0) ? hitter_->bid_volume() :
          static_cast<base::VolumeType>(hitter_->bid_volume() *
              (it->second->last.price - theo.theo) / credit);
        ask_->volume = std::min(size, hitter_->max_volume());
        ask_->price = it->second->last.price;
        ask_->instrument = it->first;
        ask_->spot = theo.spot;
        ask_->volatility = theo.volatility;
        ask_->ss_rate = theo.ss_rate;
        ask_->theo = theo.theo;
        ask_->delta = theo.delta;
        ask_->credit = credit;
        ask_->note = Proto::HitType_Name(Proto::HitType::LastTrade);
        auto ord = Message<Order>::New(ask_);
        if (!PositionManager::GetInstance()->TryFreeze(ord) && it->second->is_cover) {
          LOG_DBG << it->first->Id() << " hasn't enough position to cover";
          return;
        }
        api_->Submit(ord);
        it->second->asks.insert(ord->id);
        orders_.emplace(ord->id, ord);
        statistic_->set_orders(statistic_->orders() + 1);
        if (++it->second->ask_refills == hitter_->refill_limit()) {
          statistic_->set_ask_refills(statistic_->ask_refills() + 1);
        }
        LOG_INF << "enter order: " << ord;
      }
      if (it->second->bid_on && it->second->ask && it->second->asks.empty() &&
          it->second->last.price <= theo.theo - credit &&
          it->second->bid_refills < hitter_->refill_limit()) {
        if (unlikely(statistic_->orders() >= hitter_->order_limit())) {
          stop_("order limit broken");
          return;
        }
        auto size = base::IsEqual(credit, 0) ? hitter_->ask_volume() :
          static_cast<base::VolumeType>(hitter_->ask_volume() *
              (it->second->last.price - theo.theo) / credit);
        bid_->volume = std::min(size, hitter_->max_volume());
        bid_->instrument = it->first;
        bid_->price = it->second->last.price;
        bid_->spot = theo.spot;
        bid_->volatility = theo.volatility;
        bid_->ss_rate = theo.ss_rate;
        bid_->theo = theo.theo;
        bid_->delta = theo.delta;
        bid_->credit = credit;
        bid_->note = Proto::HitType_Name(Proto::HitType::LastTrade);
        auto ord = Message<Order>::New(bid_);
        if (!PositionManager::GetInstance()->TryFreeze(ord) && it->second->is_cover) {
          LOG_DBG << it->first->Id() << " hasn't enough position to cover";
          return;
        }
        api_->Submit(ord);
        it->second->bids.insert(ord->id);
        orders_.emplace(ord->id, ord);
        statistic_->set_orders(statistic_->orders() + 1);
        if (++it->second->bid_refills == hitter_->refill_limit()) {
          statistic_->set_bid_refills(statistic_->bid_refills() + 1);
        }
        LOG_INF << "enter order: " << ord;
      }
    }
  }
}

void Hitter::Evaluate(ParameterMap::iterator &it, Proto::HitType type) {
  assert (type != Proto::HitType::LastTrade);
  if (it->second->theos && it->first->Status() == Proto::InstrumentStatus::Trading) {
    double spot = dm_->GetUnderlyingTheo();
    TheoData theo;
    if (it->second->theos->FindTheo(spot, theo)) {
      auto credit = (type == Proto::HitType::Refresh) ?
        it->second->credit * 1.5 : it->second->credit;
      if (exception_ && exception_->on()) {
        credit *= (it->second->multiplier * exception_->multiplier());
      }
      if (it->second->ask_on && it->second->bid && it->second->bids.empty() &&
          it->second->bid.price >= theo.theo + credit) {
        auto size = base::IsEqual(credit, 0) ? hitter_->bid_volume() :
          static_cast<base::VolumeType>(hitter_->bid_volume() *
              (it->second->bid.price - theo.theo) / credit);
        size = std::min({size, hitter_->max_volume(), it->second->bid.volume});
        ask_->price = it->second->bid.price;
        ask_->instrument = it->first;
        ask_->spot = theo.spot;
        ask_->volatility = theo.volatility;
        ask_->ss_rate = theo.ss_rate;
        ask_->theo = theo.theo;
        ask_->delta = theo.delta;
        ask_->credit = credit;
        ask_->note = Proto::HitType_Name(type);
        while (size > 0 && it->second->ask_refills < hitter_->refill_limit()) {
          if (unlikely(statistic_->orders() >= hitter_->order_limit())) {
            stop_("order limit broken");
            return;
          }
          auto ord = Message<Order>::New(ask_);
          ord->volume = std::min(size, max_volume_);
          if (it->second->is_cover && !PositionManager::GetInstance()->TryFreeze(ord)) {
            LOG_DBG << it->first->Id() << " hasn't enough position to cover";
            return;
          }
          api_->Submit(ord);
          it->second->asks.insert(ord->id);
          statistic_->set_orders(statistic_->orders() + 1);
          if (++it->second->ask_refills == hitter_->refill_limit()) {
            statistic_->set_ask_refills(statistic_->ask_refills() + 1);
          }
          size -= ord->volume;
          it->second->bid.volume -= ord->volume;
          LOG_INF << "enter order: " << ord;
        }
      }
      if (it->second->bid_on && it->second->ask && it->second->asks.empty() &&
          it->second->ask.price <= theo.theo - credit) {
        auto size = base::IsEqual(credit, 0) ? hitter_->ask_volume() :
          static_cast<base::VolumeType>(hitter_->ask_volume() *
              (it->second->ask.price - theo.theo) / credit);
        size = std::min({size, hitter_->max_volume(), it->second->ask.volume});
        bid_->instrument = it->first;
        bid_->price = it->second->ask.price;
        bid_->spot = theo.spot;
        bid_->volatility = theo.volatility;
        bid_->ss_rate = theo.ss_rate;
        bid_->theo = theo.theo;
        bid_->delta = theo.delta;
        bid_->credit = credit;
        bid_->note = Proto::HitType_Name(type);
        while (size > 0 && it->second->bid_refills < hitter_->refill_limit()) {
          if (unlikely(statistic_->orders() >= hitter_->order_limit())) {
            stop_("order limit broken");
            return;
          }
          auto ord = Message<Order>::New(bid_);
          ord->volume = std::min(size, max_volume_);
          if (it->second->is_cover && !PositionManager::GetInstance()->TryFreeze(ord)) {
            LOG_DBG << it->first->Id() << " hasn't enough position to cover";
            return;
          }
          api_->Submit(ord);
          it->second->bids.insert(ord->id);
          statistic_->set_orders(statistic_->orders() + 1);
          if (++it->second->bid_refills == hitter_->refill_limit()) {
            statistic_->set_bid_refills(statistic_->bid_refills() + 1);
          }
          size -= ord->volume;
          it->second->ask.volume -= ord->volume;
          LOG_INF << "enter order: " << ord;
        }
      }
    }
  }
}
