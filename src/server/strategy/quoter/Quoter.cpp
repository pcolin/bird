#include "Quoter.h"
#include <future>
#include "strategy/base/ClusterManager.h"
#include "base/common/Float.h"
#include "base/logger/Logging.h"
#include "exchange/manager/ExchangeManager.h"
#include "model/InstrumentManager.h"
#include "model/ParameterManager.h"
#include "model/PositionManager.h"
#include "model/Middleware.h"
#include "config/EnvConfig.h"
#include "boost/format.hpp"

Quoter::Quoter(const std::string &name, DeviceManager *dm)
    : Strategy(name, dm), statistic_(new Proto::StrategyStatistic()) {
  dispatcher_.RegisterCallback<Proto::PriceException>(
      std::bind(&Quoter::OnPriceException, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::RequestForQuote>(
      std::bind(&Quoter::OnRequestForQuote, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::SSRate>(
      std::bind(&Quoter::OnSSRate, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::VolatilityCurve>(
      std::bind(&Quoter::OnVolatilityCurve, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::InterestRateReq>(
      std::bind(&Quoter::OnInterestRateReq, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::Credit>(
      std::bind(&Quoter::OnCredit, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::Destriker>(
      std::bind(&Quoter::OnDestriker, this, std::placeholders::_1));
  // dispatcher_.RegisterCallback<Proto::QuoterSpec>(
  //     std::bind(&Quoter::OnQuoterSpec, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::StrategySwitch>(
      std::bind(&Quoter::OnStrategySwitch, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::StrategyOperate>(
      std::bind(&Quoter::OnStrategyOperate, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::InstrumentReq>(
      std::bind(&Quoter::OnInstrumentReq, this, std::placeholders::_1));

  side_quote_ = EnvConfig::GetInstance()->GetBool(EnvVar::SUPPORT_SIDE_QUOTE, false);
  if (side_quote_) {
    amend_quote_ = quote_ = true;
  } else {
    amend_quote_ = EnvConfig::GetInstance()->GetBool(EnvVar::SUPPORT_AMEND_QUOTE, false);
    quote_ = amend_quote_ || EnvConfig::GetInstance()->GetBool(EnvVar::SUPPORT_QUOTE);
  }

  bid_ = std::make_shared<Order>();
  bid_->strategy = name_;
  // bid_->volume = quoter_->volume();
  bid_->strategy_type = Proto::StrategyType::Quoter;
  bid_->side = Proto::Side::Buy;
  bid_->time_condition = Proto::TimeCondition::GTD;
  bid_->type = Proto::OrderType::Limit;
  bid_->status = Proto::OrderStatus::Local;

  ask_ = std::make_shared<Order>();
  ask_->strategy = name_;
  // ask_->volume = quoter_->volume();
  ask_->strategy_type = Proto::StrategyType::Quoter;
  ask_->side = Proto::Side::Sell;
  ask_->time_condition = Proto::TimeCondition::GTD;
  ask_->type = Proto::OrderType::Limit;
  ask_->status = Proto::OrderStatus::Local;

  statistic_->set_name(name_);
  statistic_->set_type(Proto::StrategyType::Quoter);
  auto *underlying = Underlying();
  statistic_->set_exchange(underlying->Exchange());
  statistic_->set_underlying(underlying->Id());
  statistic_->set_status(Proto::StrategyStatus::Running);
}

void Quoter::OnStart() {
  api_ = ExchangeManager::GetInstance()->GetTraderApi();
  assert(api_);

  quoter_ = ClusterManager::GetInstance()->FindQuoter(name_);
  if (quoter_) {
    std::unordered_map<std::string, std::tuple<double, double>> credits;
    auto tmp = ClusterManager::GetInstance()
               ->FindCredits(Proto::StrategyType::Quoter, Underlying());
    for (auto &credit : tmp) {
      for (auto &r : credit->records()) {
        credits[r.option()] = std::make_tuple(r.credit(), credit->multiplier());
      }
    }
    LOG_DBG << boost::format("Get %1% credits") % credits.size();
    parameters_.clear();
    for (auto &op : quoter_->options()) {
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
          ParameterManager::GetInstance()->GetDestriker(inst, p->destriker);
          auto sw = ClusterManager::GetInstance()->FindStrategySwitch(
              Proto::StrategyType::Quoter, inst);
          if (sw) {
            p->is_on = sw->is_bid();
            p->is_qr = sw->is_qr_cover();
          }
          p->status = inst->Status();
          parameters_.emplace(inst, p);
          LOG_INF << boost::format("add  %1%: on(%2%), qr(%3%), credit(%4%), multiplier"
              "(%5%), destriker(%6%), status(%7%)") % op % p->is_on % p->is_qr %
              p->credit % p->multiplier % p->destriker %
              Proto::InstrumentStatus_Name(p->status);
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
    LOG_ERR << "quoter " << name_ << " isn't existed";
    stop_(name_ + "isn't existed");
  }
}

void Quoter::OnStop() {
  LOG_INF << "OnStop";
  for (auto &it : parameters_) {
    CancelOrders(it.second, "stop");
  }
  Middleware::GetInstance()->Publish(
      std::make_shared<Proto::StrategyStatistic>(*statistic_));
}

void Quoter::OnPrice(const PricePtr &price) {
  LOG_DBG << price;
  if (price->instrument->Type() == Proto::InstrumentType::Option) {
    auto it = parameters_.find(price->instrument);
    if (it != parameters_.end()) {
      it->second->price = price;
    }
  } else if (price->instrument == Underlying()) {
    if (unlikely(statistic_->orders() >= quoter_->order_limit())) {
      stop_("order limit broken");
      return;
    }
    for (auto it = parameters_.begin(); it != parameters_.end(); ++it) {
      // if (unlikely(it.second->multiplier <= 0)) {
      //   LOG_INF << boost::format("credit multiplier of %1% is zero") % it.first;
      //   continue;
      // }
      if (unlikely(it->second->credit <= 0)) {
        LOG_INF << boost::format("credit of %1% is zero") % it->first->Id();
        continue;
      }
      if (unlikely(!it->second->theos)) {
        LOG_INF << boost::format("theo matrix of %1% is null") % it->first->Id();
        continue;
      }
      if (unlikely(it->second->bid_refills == quoter_->refill_limit())) {
        LOG_INF << boost::format("bid refill time of %1% is broken") % it->first->Id();
        continue;
      }
      if (unlikely(it->second->ask_refills == quoter_->refill_limit())) {
        LOG_INF << boost::format("bid refill time of %1% is broken") % it->first->Id();
        continue;
      }
      CalculateAndResubmit(it, "spot refresh");
    }
  }
}

void Quoter::OnTheoMatrix(const TheoMatrixPtr &theo) {
  LOG_DBG << "";
  auto it = parameters_.find(theo->option);
  if (unlikely(it == parameters_.end())) {
    LOG_DBG << boost::format("can't find option %1%") % theo->option->Id();
    return;
  }
  it->second->theos = theo;
  LOG_INF << "update theo matrix of " << theo->option->Id();
  if (unlikely(statistic_->orders() >= quoter_->order_limit())) {
    stop_("order limit broken");
    return;
  }
  if (unlikely(it->second->credit <= 0)) {
    LOG_INF << boost::format("credit of %1% is zero") % it->first->Id();
    return;
  }
  if (unlikely(it->second->bid_refills == quoter_->refill_limit())) {
    LOG_INF << boost::format("bid refill time of %1% is broken") % it->first->Id();
    return;
  }
  if (unlikely(it->second->ask_refills == quoter_->refill_limit())) {
    LOG_INF << boost::format("ask refill time of %1% is broken") % it->first->Id();
    return;
  }
  CalculateAndResubmit(it, "theos refresh");
}

void Quoter::OnOrder(const OrderPtr &order) {
  assert(order);
  LOG_DBG << order;
  if (order->strategy == name_ && order->instrument->Type() == Proto::Option) {
    auto it = parameters_.find(order->instrument);
    if (unlikely(it == parameters_.end())) {
      return;
    }
    auto update_order = [&](OrderPtr &side, OrderPtr &other_side) {
      assert(side);
      assert(other_side);
      if (other_side->IsInactive()) {
        other_side.reset();
        side.reset();
        it->second->canceling = false;
        it->second->qr_id.clear();
      } else {
        side = order;
      }
    };
    if (order->IsBid()) {
      auto &bid = it->second->bid;
      if (bid && bid->id == order->id) {
        switch (order->status) {
          case Proto::OrderStatus::Submitted:
          case Proto::OrderStatus::New: {
            bid = order;
            break;
          }
          case Proto::OrderStatus::PartialFilled: {
            bid = order;
            if (it->second->destriker != 0) {
              CancelOrders(it->second, "partial filled");
              it->second->theos.reset();
            } else if (it->second->is_on && Check(it)) {
              ResubmitOrders(it, "partial filled");
            }
            break;
          }
          case Proto::OrderStatus::Filled: {
            update_order(bid, it->second->ask);
            if (it->second->destriker != 0) {
              CancelOrders(it->second, "fill");
              it->second->theos.reset();
            }
            if (unlikely(++it->second->bid_refills == quoter_->refill_limit())) {
              statistic_->set_bid_refills(statistic_->bid_refills() + 1);
            } else if (it->second->is_on && Check(it)) {
              ResubmitOrders(it, "filled");
            }
            break;
          }
          case Proto::OrderStatus::PartialFilledCanceled: {
            update_order(bid, it->second->ask);
            if (unlikely(++it->second->bid_refills == quoter_->refill_limit())) {
              statistic_->set_bid_refills(statistic_->bid_refills() + 1);
            } else if (it->second->is_on && Check(it)) {
              ResubmitOrders(it, "partial canceled");
            }
            break;
          }
          case Proto::OrderStatus::Canceled: {
            update_order(bid, it->second->ask);
            if (it->second->is_on && Check(it)) {
              ResubmitOrders(it, "canceled");
            }
            orders_.erase(order->id);
            break;
          }
          case Proto::OrderStatus::Rejected: {
            update_order(bid, it->second->ask);
            /// stop requote when order is rejected untill replay
            if (it->second->bid_refills < quoter_->refill_limit()) {
              it->second->bid_refills = quoter_->refill_limit();
              statistic_->set_bid_refills(statistic_->bid_refills() + 1);
            }
            orders_.erase(order->id);
            break;
          }
          default:
            break;
        }
        LOG_INF << "bid order update: " << order;
      }
    } else {
      auto &ask = it->second->ask;
      if (ask && ask->id == order->id) {
        switch (order->status) {
          case Proto::OrderStatus::Submitted:
          case Proto::OrderStatus::New: {
            ask = order;
            break;
          }
          case Proto::OrderStatus::PartialFilled: {
            ask = order;
            if (it->second->destriker != 0) {
              CancelOrders(it->second, "partial filled");
              it->second->theos.reset();
            } else if (it->second->is_on && Check(it)) {
              ResubmitOrders(it, "partial filled");
            }
            break;
          }
          case Proto::OrderStatus::Filled: {
            update_order(ask, it->second->bid);
            if (it->second->destriker != 0) {
              CancelOrders(it->second, "fill");
              it->second->theos.reset();
            }
            if (unlikely(++it->second->ask_refills == quoter_->refill_limit())) {
              statistic_->set_ask_refills(statistic_->ask_refills() + 1);
            } else if (it->second->is_on && Check(it)) {
              ResubmitOrders(it, "filled");
            }
            break;
          }
          case Proto::OrderStatus::PartialFilledCanceled: {
            update_order(ask, it->second->bid);
            if (unlikely(++it->second->ask_refills == quoter_->refill_limit())) {
              statistic_->set_ask_refills(statistic_->ask_refills() + 1);
            } else if (it->second->is_on && Check(it)) {
              ResubmitOrders(it, "partial canceled");
            }
            break;
          }
          case Proto::OrderStatus::Canceled: {
            update_order(ask, it->second->bid);
            if (it->second->is_on && Check(it)) {
              ResubmitOrders(it, "canceled");
            }
            orders_.erase(order->id);
            break;
          }
          case Proto::OrderStatus::Rejected: {
            update_order(ask, it->second->bid);
            /// stop requote when order is rejected untill replay
            if (it->second->ask_refills < quoter_->refill_limit()) {
              it->second->ask_refills = quoter_->refill_limit();
              statistic_->set_ask_refills(statistic_->ask_refills() + 1);
            }
            orders_.erase(order->id);
            break;
          }
          default:
            break;
        }
        LOG_INF << "ask order update: " << order;
      }
    }
  }
}

void Quoter::OnTrade(const TradePtr &trade) {
  LOG_DBG << trade;
  auto it = orders_.find(trade->order_id);
  if (it != orders_.end()) {
    statistic_->set_trades(statistic_->trades() + 1);
    if (statistic_->trades() >= quoter_->trade_limit()) {
      stop_("trade limit broken");
      return;
    }
    statistic_->set_delta(statistic_->delta() +
        it->second->delta * trade->volume * trade->instrument->Multiplier() /
        trade->instrument->Underlying()->Multiplier());
    if (base::IsMoreThan(statistic_->delta(), quoter_->delta_limit())) {
      stop_("delta limit broken");
      return;
    }
  }
}

bool Quoter::OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat) {
  LOG_DBG << heartbeat->ShortDebugString();
  CheckForAuction();
  CheckForQR();
  Middleware::GetInstance()->Publish(
      std::make_shared<Proto::StrategyStatistic>(*statistic_));
}

bool Quoter::OnPriceException(const std::shared_ptr<Proto::PriceException> &msg) {
  if (msg->on()) {
    stop_("price exception");
  }
}

bool Quoter::OnRequestForQuote(const std::shared_ptr<Proto::RequestForQuote> &msg) {
  LOG_DBG << msg->ShortDebugString();
  if (quoter_->qr_delay() > 0) {
    pending_rfqs_.push(std::make_tuple(msg, base::Now()));
  } else {
    RespondingQR(msg);
  }
}

bool Quoter::OnSSRate(const std::shared_ptr<Proto::SSRate> &msg) {
  LOG_DBG << msg->ShortDebugString();
  auto maturity = boost::gregorian::from_undelimited_string(msg->maturity());
  for (auto &it : parameters_) {
    if (it.first->Maturity() == maturity) {
      CancelOrders(it.second, "ssr refresh");
      it.second->theos.reset();
    }
  }
  // return true;
}

bool Quoter::OnCredit(const std::shared_ptr<Proto::Credit> &msg) {
  LOG_DBG << msg->ShortDebugString();
  if (msg->strategy() == Proto::StrategyType::Quoter) {
    for (auto &r : msg->records()) {
      auto *inst = InstrumentManager::GetInstance()->FindId(r.option());
      if (inst) {
        auto it = parameters_.find(inst);
        if (it != parameters_.end()) {
          it->second->multiplier = msg->multiplier();
          it->second->credit = r.credit();
          if (unlikely(statistic_->orders() >= quoter_->order_limit())) {
            stop_("order limit broken");
            continue;
          }
          if (unlikely(it->second->bid_refills == quoter_->refill_limit())) {
            LOG_INF << boost::format("bid refill time of %1% is broken") % inst->Id();
            continue;
          }
          if (unlikely(it->second->ask_refills == quoter_->refill_limit())) {
            LOG_INF << boost::format("ask refill time of %1% is broken") % inst->Id();
            continue;
          }
          if (unlikely(!it->second->theos)) {
            LOG_INF << boost::format("theos of %1% is null") % inst->Id();
            continue;
          }
          CalculateAndResubmit(it, "credit refresh");
        }
      }
    }
  }
}

bool Quoter::OnDestriker(const std::shared_ptr<Proto::Destriker> &msg) {
  LOG_INF << "Destriker: " << msg->ShortDebugString();
  auto *inst = InstrumentManager::GetInstance()->FindId(msg->instrument());
  assert (inst);
  auto it = parameters_.find(inst);
  if (it != parameters_.end()) {
    CancelOrders(it->second, "destriker refresh");
    it->second->destriker = msg->destriker();
    it->second->theos.reset();
  }
}

bool Quoter::OnVolatilityCurve(const std::shared_ptr<Proto::VolatilityCurve> &msg) {
  LOG_DBG << msg->ShortDebugString();
  auto maturity = boost::gregorian::from_undelimited_string(msg->maturity());
  for (auto &it : parameters_) {
    if (it.first->Maturity() == maturity) {
      CancelOrders(it.second, "vol refresh");
      it.second->theos.reset();
    }
  }
}

bool Quoter::OnInterestRateReq(const std::shared_ptr<Proto::InterestRateReq> &msg) {
  LOG_DBG << msg->ShortDebugString();
  for (auto &it : parameters_) {
    CancelOrders(it.second, "rate refresh");
    it.second->theos.reset();
  }
}

bool Quoter::OnStrategySwitch(const std::shared_ptr<Proto::StrategySwitch> &sw) {
  LOG_DBG << sw->ShortDebugString();
  if (sw->strategy() == Proto::StrategyType::Quoter) {
    auto *inst = InstrumentManager::GetInstance()->FindId(sw->option());
    if (inst) {
      auto it = parameters_.find(inst);
      if (it != parameters_.end()) {
        if (it->second->is_on != sw->is_bid()) {
          if (it->second->is_on) {
            /// true --> false
            CancelOrders(it->second, "switch off");
          } else if (Check(it)) {
            /// false --> true
            ResubmitOrders(it, "switch on");
          }
          it->second->is_on = sw->is_bid();
        }
        it->second->is_qr = sw->is_qr_cover();
      }
    }
  }
}

bool Quoter::OnStrategyOperate(const std::shared_ptr<Proto::StrategyOperate> &msg) {
  if (msg->name() == Name() && msg->operate() == Proto::StrategyOperation::Start) {
    statistic_->clear_delta();
    statistic_->clear_orders();
    statistic_->clear_trades();
    statistic_->clear_bid_refills();
    statistic_->clear_ask_refills();
    for (auto it = parameters_.begin(); it != parameters_.end(); ++it) {
        it->second->bid_refills = it->second->ask_refills = 0;
        if (unlikely(it->second->credit <= 0)) {
          LOG_INF << boost::format("credit of %1% is zero") % it->first->Id();
          continue;
        }
        if (unlikely(!it->second->theos)) {
          LOG_INF << boost::format("theos of %1% is null") % it->first->Id();
          continue;
        }
        CalculateAndResubmit(it, "start");
    }
  }
}

bool Quoter::OnInstrumentReq(const std::shared_ptr<Proto::InstrumentReq> &msg) {
  LOG_INF << "InstrumentReq: " << msg->ShortDebugString();
  for (auto &inst : msg->instruments()) {
    if (inst.type() == Proto::InstrumentType::Option) {
      auto *instrument = InstrumentManager::GetInstance()->FindId(inst.id());
      if (instrument && instrument->HedgeUnderlying () == Underlying()) {
        auto it = parameters_.find(instrument);
        if (unlikely(it == parameters_.end())) {
          return false;
        }
        auto status = inst.status();
        if (status == Proto::InstrumentStatus::OpeningAuction) {
          // CancelOrders(it->second, "");
          it->second->auction_time = base::Now() +
            quoter_->open_auction_delay() * base::MILLION;
          it->second->auction_volume = quoter_->open_auction_volume();
        } else if (status == Proto::InstrumentStatus::ClosingAuction) {
          CancelOrders(it->second, "close auction");
          it->second->auction_time = base::Now() +
            quoter_->close_auction_delay() * base::MILLION;
          it->second->auction_volume = quoter_->close_auction_volume();
        } else if (status == Proto::InstrumentStatus::Fuse) {
          CancelOrders(it->second, "fuse");
          it->second->auction_time = base::Now() +
            quoter_->fuse_auction_delay() * base::MILLION;
          it->second->auction_volume = quoter_->fuse_auction_volume();
        }
        it->second->status = status;
      }
    }
  }
}

void Quoter::CheckForAuction() {
  auto now = base::Now();
  for (auto it = parameters_.begin(); it != parameters_.end(); ++it) {
    if (Instrument::IsAuction(it->second->status) && it->second->auction_time >= now) {
      if (unlikely(it->second->credit <= 0)) {
        LOG_INF << boost::format("credit of %1% is zero") % it->first->Id();
        continue;
      }
      if (unlikely(!it->second->theos)) {
        LOG_INF << boost::format("theos of %1% is null") % it->first->Id();
        continue;
      }
      ResubmitOrders(it, "auction");
    }
  }
}

void Quoter::CheckForQR() {
  auto now = base::Now();
  while (pending_rfqs_.size() > 0) {
    auto &t = pending_rfqs_.front();
    if (std::get<1>(t) >= now) {
      RespondingQR(std::get<0>(t));
      pending_rfqs_.pop();
    } else {
      break;
    }
  }
}

void Quoter::RespondingQR(const std::shared_ptr<Proto::RequestForQuote> &rfq) {
  LOG_INF << boost::format("responding rfq(%1%) of %2%") % rfq->id() % rfq->instrument();
  auto *instrument = InstrumentManager::GetInstance()->FindId(rfq->instrument());
  assert(instrument);
  auto it = parameters_.find(instrument);
  if (it != parameters_.end() && it->second->is_qr) {
    if (it->second->bid) {
      CancelOrders(it->second, "rfq");
      it->second->qr_id = rfq->id();
    } else {
      it->second->qr_id = rfq->id();
      if (Check(it)) {
        ResubmitOrders(it, "rfq");
      }
    }
  }
}

bool Quoter::Check(ParameterMap::iterator &it) {
  if (unlikely(statistic_->orders() >= quoter_->order_limit())) {
    stop_("order limit broken");
    return false;
  }
  if (unlikely(it->second->bid_refills == quoter_->refill_limit())) {
    LOG_INF << boost::format("bid refill time of %1% is broken") % it->first->Id();
    return false;
  }
  if (unlikely(it->second->ask_refills == quoter_->refill_limit())) {
    LOG_INF << boost::format("ask refill time of %1% is broken") % it->first->Id();
    return false;
  }
  if (unlikely(it->second->credit <= 0)) {
    LOG_INF << boost::format("credit of %1% is zero") % it->first->Id();
    return false;
  }
  if (unlikely(!it->second->theos)) {
    LOG_INF << boost::format("theos of %1% is null") % it->first->Id();
    return false;
  }
  if (unlikely(it->second->status != Proto::InstrumentStatus::Trading)) {
    LOG_INF << it->first->Id() << " is in " <<
      Proto::InstrumentStatus_Name(it->first->Status());
    return false;
  }
  return true;
}

void Quoter::CalculateAndResubmit(ParameterMap::iterator &it, const char *reason) {
  if (it->second->is_on) {
    if (it->second->status == Proto::InstrumentStatus::Trading) {
      ResubmitOrders(it, reason);
    } else {
      CancelOrders(it->second, "not trading");
    }
  } else {
    CancelOrders(it->second, "switch off");
  }
}

void Quoter::ResubmitOrders(ParameterMap::iterator &it, const char *reason) {
  double spot = dm_->GetUnderlyingTheo();
  TheoData theo;
  if (unlikely(!it->second->theos->FindTheo(spot, theo))) {
    CancelOrders(it->second, "invalid theo");
    LOG_INF << boost::format("can't find theo of %1% by spot %2%") %
      it->first->Id() % spot;
    return;
  }
  LOG_DBG << boost::format("find theo(%1%) of %2% by spot %3%") %
    theo.theo % it->first->Id() % spot;
  // double theo = it->second->theo.theo + it->second->destriker * it->second->position;
  double bp = it->first->RoundToTick(theo.theo - (1 + 0.5 * it->second->multiplier) *
      it->second->credit, Proto::RoundDirection::Down);
  double ap = it->first->RoundToTick(theo.theo + (1 + 0.5 * it->second->multiplier) *
      it->second->credit, Proto::RoundDirection::Up);
  double tolerance_ratio = 1;
  if (!quoter_->wide_spread()) {
    double mm_bid = 0, mm_ask = 0;
    if ((it->second->qr_id.empty() &&
         api_->GetMMPrice(it->first, theo.theo, mm_bid, mm_ask)) ||
        (!it->second->qr_id.empty() &&
         api_->GetQRPrice(it->first, theo.theo, mm_bid, mm_ask))) {
      if (base::IsMoreThan(mm_bid, bp)) {
        tolerance_ratio = (mm_ask - mm_bid) / (ap - bp);
        bp = mm_bid;
        if (base::IsLessThan(mm_ask, ap) || base::IsLessThan(ap, bp)) {
          ap = mm_ask;
        }
      } else if (base::IsLessThan(mm_ask, ap)) {
        tolerance_ratio = (mm_ask - mm_bid) / (ap - bp);
        ap = mm_ask;
        if (base::IsMoreThan(bp, ap)) {
          bp = mm_bid;
        }
      }
    }
  }
  LOG_DBG << boost::format("get price(%1%, %2%), theo(%3%), credit(%4%), multiplier(%5%)")
      % bp % ap % theo.theo % it->second->credit % it->second->multiplier;
  if (base::IsLessThan(bp, it->first->Lowest())) {
    if (base::IsLessThan(it->first->Lowest(), theo.theo - it->second->credit)) {
      LOG_INF << boost::format("move bid price of %1% from %2% up to %3%") %
                 it->first->Id() % bp % it->first->Lowest();
      bp = it->first->Lowest();
    } else {
      LOG_INF << boost::format("%1%\'s lowest(%2%) >= theo(%3%) - credit(%4%)") %
                 it->first->Id() % it->first->Lowest() % theo.theo % it->second->credit;
      CancelOrders(it->second, "down limit");
      return;
    }
  } else if (base::IsMoreThan(bp, it->first->Highest())) {
    LOG_INF << boost::format("%1% bid price(%2%) > Highest(%3%)") %
               it->first->Id() % bp % it->first->Highest();
    CancelOrders(it->second, "up limit");
    return;
  }
  if (base::IsMoreThan(ap, it->first->Highest())) {
    if (base::IsMoreThan(it->first->Highest(), theo.theo + it->second->credit)) {
      LOG_INF << boost::format("move ask price of %1% from %2% down to %3%") %
                 it->first->Id() % ap % it->first->Lowest();
      ap = it->first->Highest();
    } else {
      LOG_INF << boost::format("%1%\'s highest(%2%) <= theo(%3%) + credit(%4%)") %
                 it->first->Id() % it->first->Highest() % theo.theo % it->second->credit;
      CancelOrders(it->second, "up limit");
      return;
    }
  } else if (base::IsLessThan(ap, it->first->Lowest())) {
    LOG_INF << boost::format("%1% ask price(%2%) < Lowest(%3%)") %
               it->first->Id() % ap % it->first->Lowest();
    CancelOrders(it->second, "down limit");
    return;
  }
  auto &price = it->second->price;
  if (quoter_->protection() && price) {
    if (price->asks[0] && bp >= price->asks[0].price) {
      LOG_DBG << boost::format("protect mode works(%1%), bid(%2%) >= market ask(%3% %4%)")
                 % it->first->Id() % bp % price->asks[0].price % price->asks[0].volume;
      CancelOrders(it->second, "market ask cross");
      return;
    }
    if (price->bids[0] && ap <= price->bids[0].price) {
      LOG_DBG << boost::format("protect mode works(%1%), ask(%2%) <= market bid(%3% %4%)")
                 % it->first->Id() % ap % price->bids[0].price % price->bids[0].volume;
      CancelOrders(it->second, "cross market bid");
      return;
    }
  }
  /// check tolerance
  double tolerance = 0.5 * it->second->multiplier * it->second->credit * tolerance_ratio;
  if (it->second->bid &&
      it->second->bid->status < Proto::PartialFilled &&
      it->second->ask->status < Proto::PartialFilled &&
      base::IsBetween(it->second->bid->price, bp - tolerance, bp + tolerance) &&
      base::IsBetween(it->second->ask->price, ap - tolerance, ap + tolerance)) {
    LOG_DBG << boost::format("orders in tolerance(%1%): theo(%2%), "
                             "bid(%3%, %4%, %5%), ask(%6%, %7%, %8%)") %
               it->first->Id() % theo.theo %
               (bp - tolerance) % it->second->bid->price % (bp + tolerance) %
               (ap - tolerance) % it->second->ask->price % (ap + tolerance);
    return;
  }

  auto build_orders = [&]() {
    bid_->instrument = ask_->instrument = it->first;
    bid_->price = bp;
    ask_->price = ap;
    if (it->second->status == Proto::InstrumentStatus::Trading) {
      if (it->second->qr_id.empty()) {
        bid_->volume = ask_->volume = quoter_->volume();
      } else {
        bid_->volume = ask_->volume = quoter_->qr_volume();
      }
    } else {
      assert (Instrument::IsAuction(it->second->status));
      bid_->volume = ask_->volume = it->second->auction_volume;
    }
    bid_->spot = ask_->spot = theo.spot;
    bid_->volatility = ask_->volatility = theo.volatility;
    bid_->ss_rate = ask_->ss_rate = theo.ss_rate;
    bid_->theo = ask_->theo = theo.theo;
    bid_->delta = ask_->delta = theo.delta;
    bid_->credit = ask_->credit = it->second->credit;
    it->second->bid = std::make_shared<Order>(*bid_);
    it->second->bid->header.SetTime();
    it->second->bid->ResetId();
    orders_.emplace(it->second->bid->id, it->second->bid);
    PositionManager::GetInstance()->TryFreeze(it->second->bid);
    it->second->ask = std::make_shared<Order>(*ask_);
    it->second->ask->header.SetTime();
    it->second->ask->ResetId();
    orders_.emplace(it->second->ask->id, it->second->ask);
    PositionManager::GetInstance()->TryFreeze(it->second->ask);
    statistic_->set_orders(statistic_->orders() + 2);
  };

  if (it->second->bid) {
    assert(it->second->ask);
    if (amend_quote_) {
      build_orders();
      api_->Submit(it->second->bid, it->second->ask);
      LOG_INF << "amend quote(" << reason << "): " <<
        it->second->bid << ", " << it->second->ask;
    } else {
      CancelOrders(it->second, "resubmit");
    }
  } else {
    assert(!it->second->ask);
    build_orders();
    if (quote_) {
      api_->Submit(it->second->bid, it->second->ask);
    } else {
      api_->Submit(it->second->bid);
      api_->Submit(it->second->ask);
    }
    LOG_INF << "new quote(" << reason << "): " <<
      it->second->bid << ", " << it->second->ask;
  }
}

void Quoter::CancelOrders(const ParameterPtr &parameter, const char *reason) {
  if (parameter->canceling) {
    LOG_INF << "quote is canceling: " << parameter->bid << ", " << parameter->ask;
  } else if (!parameter->qr_id.empty()) {
    LOG_INF << "forbid canceling qr quote: " << parameter->bid << ", " <<  parameter->ask;
  } else if (parameter->bid) {
    assert(parameter->ask);
    if (parameter->status == Proto::InstrumentStatus::Trading) {
      if (quote_) {
        api_->Cancel(parameter->bid, parameter->ask);
        LOG_INF << "cancel quote(" << reason << "): " <<
            parameter->bid << ", " << parameter->ask;
      } else {
        if (!parameter->bid->IsInactive()) {
          api_->Cancel(parameter->bid);
          LOG_INF << "cancel bid(" << reason << "): " << parameter->bid;
          if (!parameter->ask->IsInactive()) {
            api_->Cancel(parameter->ask);
            LOG_INF << "cancel ask(" << reason << "): " << parameter->ask;
          }
        } else {
          assert(!parameter->ask->IsInactive());
          api_->Cancel(parameter->ask);
          LOG_INF << "cancel ask(" << reason << "): " << parameter->ask;
        }
      }
      parameter->canceling = true;
    }
  } else {
    assert(!parameter->ask);
  }
}
