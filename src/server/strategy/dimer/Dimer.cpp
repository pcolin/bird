#include "Dimer.h"
#include "base/common/Float.h"
#include "model/Middleware.h"
#include "model/InstrumentManager.h"
#include "model/PositionManager.h"
#include "model/ParameterManager.h"
#include "strategy/base/ClusterManager.h"
#include "exchange/manager/ExchangeManager.h"

Dimer::Dimer(const std::string &name, DeviceManager *dm)
    : Strategy(name, dm),
      bid_(Message<Order>::New()), ask_(Message<Order>::New()),
      statistic_(Message<Proto::StrategyStatistic>::New()) {
  dispatcher_.RegisterCallback<Proto::PriceException>(
      std::bind(&Dimer::OnPriceException, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::SSRate>(
      std::bind(&Dimer::OnSSRate, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::VolatilityCurve>(
      std::bind(&Dimer::OnVolatilityCurve, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::InterestRateReq>(
      std::bind(&Dimer::OnInterestRateReq, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::Credit>(
      std::bind(&Dimer::OnCredit, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::Destriker>(
      std::bind(&Dimer::OnDestriker, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::StrategySwitch>(
      std::bind(&Dimer::OnStrategySwitch, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::StrategyOperate>(
      std::bind(&Dimer::OnStrategyOperate, this, std::placeholders::_1));

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
  statistic_->set_type(Proto::StrategyType::Dimer);
  auto *underlying = Underlying();
  statistic_->set_exchange(underlying->Exchange());
  statistic_->set_underlying(underlying->Id());
  statistic_->set_status(Proto::StrategyStatus::Running);
}

void Dimer::OnStart() {
  api_ = ExchangeManager::GetInstance()->GetTraderApi();
  assert(api_);

  dimer_ = ClusterManager::GetInstance()->FindDimer(name_);
  if (dimer_) {
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
    for (auto &op : dimer_->options()) {
      auto *inst = InstrumentManager::GetInstance()->FindId(op);
      if (inst) {
        auto it = parameters_.find(inst);
        if (it == parameters_.end()) {
          auto p = std::make_shared<Parameter>();
          auto itr = credits.find(op);
          if (itr != credits.end()) {
            p->credit = std::get<0>(itr->second);
          }
          ParameterManager::GetInstance()->GetDestriker(inst, p->destriker);
          auto sw = ClusterManager::GetInstance()->FindStrategySwitch(
              Proto::StrategyType::Dimer, inst);
          if (sw) {
            p->bid.on = sw->is_bid();
            p->ask.on = sw->is_ask();
            p->cover = sw->is_qr_cover();
          }
          parameters_.emplace(inst, p);
        } else {
          LOG_WAN << "duplicate option " << op << " in " << name_;
        }
      }
    }

    bid_->volume = dimer_->bid_volume();
    ask_->volume = dimer_->ask_volume();

    orders_.clear();
    statistic_->clear_delta();
    statistic_->clear_orders();
    statistic_->clear_trades();
    statistic_->clear_bid_refills();
    statistic_->clear_ask_refills();
  } else {
    LOG_ERR << "dimer " << name_ << " isn't existed";
    stop_(name_ + "isn't existed");
  }
}

void Dimer::OnStop() {
  // LOG_INF << "OnStop";
  for (auto &it : parameters_) {
    if (it.second->bid.order) {
      Cancel(it.second->bid, "stop");
    }
    if (it.second->ask.order) {
      Cancel(it.second->ask, "stop");
    }
  }
  Middleware::GetInstance()->Publish(Message<Proto::StrategyStatistic>::New(*statistic_));
}

void Dimer::OnPrice(const PricePtr &price) {
  LOG_DBG << price;
  if (price->instrument->Type() == Proto::InstrumentType::Option) {
    auto it = parameters_.find(price->instrument);
    if (unlikely(it == parameters_.end())) {
      return;
    }
    // it->second->price = price;
    if (it->second->bid.order) {
      if (!price->bids[0] ||
          base::IsLessThan(it->second->bid.order->price, price->bids[0].price)) {
        if (!it->second->bid.canceling) {
          api_->Cancel(it->second->bid.order);
          it->second->bid.canceling = true;
          LOG_INF << "cancel order(price refresh): " << it->second->bid.order;
        } else {
          LOG_INF << "order is canceling: " << it->second->bid.order;
        }
      }
    } else if (it->second->bid.on && price->bids[0] && it->second->theos &&
        it->second->bid.refills < dimer_->refill_limit() &&
        it->first->Status() == Proto::InstrumentStatus::Trading &&
        (it->second->bid.other_orders.empty() ||
         base::IsMoreThan(price->bids[0].price,
           it->second->bid.other_orders.front()->price))) {
      double spot = dm_->GetUnderlyingTheo();
      TheoData t;
      if (it->second->theos->FindTheo(spot, t)) {
        for (auto i = dimer_->tick(); i >= 0; --i) {
          auto p = it->first->RoundToTick(price->bids[0].price + i * it->first->Tick(),
              Proto::RoundDirection::Nearest);
          if (base::IsLessOrEqual(p, t.theo - it->second->credit) &&
              base::IsBetween(p, it->first->Lowest(), it->first->Highest())) {
            if (unlikely(statistic_->orders() >= dimer_->order_limit())) {
              stop_("order limit broken");
              return;
            }
            bid_->instrument = it->first;
            bid_->price = p;
            bid_->spot = t.spot;
            bid_->volatility = t.volatility;
            bid_->ss_rate = t.ss_rate;
            bid_->theo = t.theo;
            bid_->delta = t.delta;
            bid_->credit = it->second->credit;
            auto ord = Message<Order>::New(bid_);
            if (!PositionManager::GetInstance()->TryFreeze(ord) && it->second->cover) {
              LOG_DBG << it->first->Id() << " hasn't enough position to cover";
              break;
            }
            api_->Submit(ord);
            it->second->bid.order = ord;
            // it->second->redim = false;
            orders_.emplace(ord->id, ord);
            statistic_->set_orders(statistic_->orders() + 1);
            LOG_INF << "enter order: " << ord;
            break;
          }
        }
      }
    }
    if (it->second->ask.order) {
      if (!price->asks[0] ||
          base::IsMoreThan(it->second->ask.order->price, price->asks[0].price)) {
        if (!it->second->ask.canceling) {
          api_->Cancel(it->second->ask.order);
          it->second->ask.canceling = true;
          LOG_INF << "cancel order(price refresh): " << it->second->ask.order;
        } else {
          LOG_INF << "order is canceling: " << it->second->ask.order;
        }
      }
    } else if (it->second->ask.on && price->asks[0] && it->second->theos &&
        it->second->ask.refills < dimer_->refill_limit() &&
        it->first->Status() == Proto::InstrumentStatus::Trading &&
        (it->second->ask.other_orders.empty() ||
         base::IsLessThan(price->asks[0].price,
           it->second->ask.other_orders.front()->price))) {
      double spot = dm_->GetUnderlyingTheo();
      TheoData t;
      if (it->second->theos->FindTheo(spot, t)) {
        for (auto i = dimer_->tick(); i >= 0; --i) {
          auto p = it->first->RoundToTick(price->asks[0].price - i * it->first->Tick(),
              Proto::RoundDirection::Nearest);
          if (base::IsMoreOrEqual(p, t.theo + it->second->credit) &&
              base::IsBetween(p, it->first->Lowest(), it->first->Highest())) {
            if (unlikely(statistic_->orders() >= dimer_->order_limit())) {
              stop_("order limit broken");
              return;
            }
            ask_->instrument = it->first;
            ask_->price = p;
            ask_->spot = t.spot;
            ask_->volatility = t.volatility;
            ask_->ss_rate = t.ss_rate;
            ask_->theo = t.theo;
            ask_->delta = t.delta;
            ask_->credit = it->second->credit;
            auto ord = Message<Order>::New(ask_);
            if (!PositionManager::GetInstance()->TryFreeze(ord) && it->second->cover) {
              LOG_DBG << it->first->Id() << " hasn't enough position to cover";
              break;
            }
            api_->Submit(ord);
            it->second->ask.order = ord;
            // it->second->redim = false;
            orders_.emplace(ord->id, ord);
            statistic_->set_orders(statistic_->orders() + 1);
            LOG_INF << "enter order: " << ord;
            break;
          }
        }
      }
    }
  } else if (price->instrument == Underlying()) {
    for (auto &it : parameters_) {
      Cancel(it.second->theos, it.second->bid,
          base::IsMoreThan, -it.second->credit, "ul refresh");
      Cancel(it.second->theos, it.second->ask,
          base::IsLessThan, it.second->credit, "ul refresh");
    }
  }
}

void Dimer::OnTheoMatrix(const TheoMatrixPtr &theo) {
  LOG_TRA << theo;
  auto it = parameters_.find(theo->option);
  if (it != parameters_.end()) {
    it->second->theos = theo;
    Cancel(theo, it->second->bid, base::IsMoreThan, -it->second->credit, "theos update");
    Cancel(theo, it->second->ask, base::IsLessThan, it->second->credit, "theos update");
  }
}

void Dimer::OnOrder(const OrderPtr &order) {
  LOG_TRA << order;
  auto it = parameters_.find(order->instrument);
  if (unlikely(it == parameters_.end())) {
    return;
  }
  if (order->strategy == name_) {
    ParameterSide &side = order->IsBid() ? it->second->bid : it->second->ask;
    if (unlikely(!side.order || side.order->id != order->id)) {
      LOG_WAN << "unkown dimer order: " << order;
      return;
    }
    switch (order->status) {
      case Proto::OrderStatus::PartialFilled: {
        if (it->second->destriker != 0) {
          it->second->theos.reset();
        }
      }
      case Proto::OrderStatus::Submitted:
      case Proto::OrderStatus::New: {
        side.order = order;
        break;
      }
      case Proto::OrderStatus::Filled: {
        if (it->second->destriker != 0) {
          it->second->theos.reset();
        }
      }
      case Proto::OrderStatus::PartialFilledCanceled: {
        if (unlikely(++side.refills == dimer_->refill_limit())) {
          if (order->IsBid()) {
            statistic_->set_bid_refills(statistic_->bid_refills() + 1);
          } else {
            statistic_->set_ask_refills(statistic_->ask_refills() + 1);
          }
        }
        side.order.reset();
        side.canceling = false;
        break;
      }
      case Proto::OrderStatus::Rejected: {
        /// stop requote when order is rejected untill replay
        if (side.refills < dimer_->refill_limit()) {
          side.refills = dimer_->refill_limit();
          if (order->IsBid()) {
            statistic_->set_bid_refills(statistic_->bid_refills() + 1);
          } else {
            statistic_->set_ask_refills(statistic_->ask_refills() + 1);
          }
        }
      }
      case Proto::OrderStatus::Canceled: {
        side.order.reset();
        side.canceling = false;
        orders_.erase(order->id);
        break;
      }
      default:
        break;
    }
    LOG_INF << "order update: " << order;
  } else {
    auto update_order = [&](std::list<OrderPtr> &orders,
        const std::function<bool(double, double)> &comparer) {
      if (order->IsInactive()) {
        auto it = orders.begin();
        while (it != orders.end() && comparer((*it)->price, order->price)) {
          if ((*it)->id == order->id) {
            orders.erase(it);
            break;
          }
          ++it;
        }
      } else {
        auto it = orders.begin();
        while (it != orders.end() && comparer((*it)->price, order->price)) {
          if ((*it)->id == order->id) {
            *it = order;
            return;
          }
          ++it;
        }
        orders.insert(it, order);
      }
    };
    if (order->IsBid()) {
      update_order(it->second->bid.other_orders, base::IsMoreOrEqual);
    } else {
      update_order(it->second->ask.other_orders, base::IsLessOrEqual);
    }
  }
}

void Dimer::OnTrade(const TradePtr &trade) {
  LOG_DBG << trade;
  auto it = orders_.find(trade->order_id);
  if (it != orders_.end()) {
    statistic_->set_trades(statistic_->trades() + 1);
    if (statistic_->trades() >= dimer_->trade_limit()) {
      stop_("trade limit broken");
      return;
    }
    statistic_->set_delta(statistic_->delta() +
        it->second->delta * trade->volume * trade->instrument->Multiplier() /
        trade->instrument->Underlying()->Multiplier());
    if (base::IsMoreThan(statistic_->delta(), dimer_->delta_limit())) {
      stop_("delta limit broken");
      return;
    }
  }
}

bool Dimer::OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat) {
  Middleware::GetInstance()->Publish(Message<Proto::StrategyStatistic>::New(*statistic_));
}

bool Dimer::OnPriceException(const std::shared_ptr<Proto::PriceException> &msg) {
  LOG_INF << msg->ShortDebugString();
  if (msg->on()) {
    stop_("price exception");
  }
}

bool Dimer::OnSSRate(const std::shared_ptr<Proto::SSRate> &msg) {
  LOG_INF << "cancel orders due to ssrate update: " << msg->ShortDebugString();
  auto maturity = boost::gregorian::from_undelimited_string(msg->maturity());
  for (auto it = parameters_.begin(); it != parameters_.end(); ++it) {
    if (it->first->Maturity() == maturity) {
      Invalidate(it);
    }
  }
}

bool Dimer::OnVolatilityCurve(const std::shared_ptr<Proto::VolatilityCurve> &msg) {
  LOG_INF << "cancel orders due to volatility curve update: " << msg->ShortDebugString();
  auto maturity = boost::gregorian::from_undelimited_string(msg->maturity());
  for (auto it = parameters_.begin(); it != parameters_.end(); ++it) {
    if (it->first->Maturity() == maturity) {
      Invalidate(it);
    }
  }
}

bool Dimer::OnInterestRateReq(const std::shared_ptr<Proto::InterestRateReq> &msg) {
  LOG_INF << "cancel orders due to interest rate update: " << msg->ShortDebugString();
  for (auto it = parameters_.begin(); it != parameters_.end(); ++it) {
    Invalidate(it);
  }
}

bool Dimer::OnCredit(const std::shared_ptr<Proto::Credit> &msg) {
  LOG_TRA << msg->ShortDebugString();
  if (msg->strategy() == Proto::StrategyType::Dimer) {
    for (auto &r : msg->records()) {
      auto *inst = InstrumentManager::GetInstance()->FindId(r.option());
      if (inst) {
        auto it = parameters_.find(inst);
        if (it != parameters_.end()) {
          it->second->credit = r.credit();
          Cancel(it->second->theos, it->second->bid,
              base::IsMoreThan, -it->second->credit, "credit refresh");
          Cancel(it->second->theos, it->second->ask,
              base::IsLessThan, it->second->credit, "credit refresh");
        }
      }
    }
  }
}

bool Dimer::OnDestriker(const std::shared_ptr<Proto::Destriker> &msg) {
  LOG_TRA << msg->ShortDebugString();
  auto *inst = InstrumentManager::GetInstance()->FindId(msg->instrument());
  assert (inst);
  auto it = parameters_.find(inst);
  if (it != parameters_.end()) {
    it->second->destriker = msg->destriker();
    Invalidate(it);
  }
}

bool Dimer::OnStrategySwitch(const std::shared_ptr<Proto::StrategySwitch> &msg) {
  LOG_DBG << msg->ShortDebugString();
  if (msg->strategy() == Proto::StrategyType::Dimer) {
    auto *inst = InstrumentManager::GetInstance()->FindId(msg->option());
    if (inst) {
      auto it = parameters_.find(inst);
      if (it != parameters_.end()) {
        if (!msg->is_bid() && it->second->bid.order) {
          Cancel(it->second->bid, "switch off");
        }
        if (!msg->is_ask() && it->second->ask.order) {
          Cancel(it->second->ask, "switch off");
        }
        it->second->bid.on = msg->is_bid();
        it->second->ask.on = msg->is_ask();
        it->second->cover = msg->is_qr_cover();
      }
    }
  }
}

bool Dimer::OnStrategyOperate(const std::shared_ptr<Proto::StrategyOperate> &msg) {
  if (msg->name() == Name() && msg->operate() == Proto::StrategyOperation::Start) {
    statistic_->clear_delta();
    statistic_->clear_orders();
    statistic_->clear_trades();
    statistic_->clear_bid_refills();
    statistic_->clear_ask_refills();
    for (auto it = parameters_.begin(); it != parameters_.end(); ++it) {
      it->second->bid.refills = it->second->ask.refills = 0;
    }
  }
}

void Dimer::Cancel(const TheoMatrixPtr &theos, ParameterSide &side,
    const std::function<bool(double, double)> &func, double credit, const char *reason) {
  if (side.order) {
    double spot = dm_->GetUnderlyingTheo();
    TheoData t;
    if (!theos || !theos->FindTheo(spot, t) || func(side.order->price, t.theo + credit)) {
      Cancel(side, reason);
    }
  }
}

void Dimer::Cancel(ParameterSide &side, const char *reason) {
  if (!side.canceling) {
    api_->Cancel(side.order);
    side.canceling = true;
    LOG_INF << "cancel order(" << reason << "): " << side.order;
  } else {
    LOG_INF << "order is canceling : " << side.order;
  }
}

void Dimer::Invalidate(ParameterMap::iterator &it) {
  if (it->second->bid.order) {
    Cancel(it->second->bid, "invalidate");
  }
  if (it->second->ask.order) {
    Cancel(it->second->ask, "invalidate");
  }
  it->second->theos.reset();
}
