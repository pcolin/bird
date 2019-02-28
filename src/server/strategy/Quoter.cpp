#include "Quoter.h"
#include <future>
#include "ClusterManager.h"
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
    : Strategy(name, dm) {
  dispatcher_.RegisterCallback<Proto::RequestForQuote>(
      std::bind(&Quoter::OnRequestForQuote, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::SSRate>(
      std::bind(&Quoter::OnSSRate, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::Credit>(
      std::bind(&Quoter::OnCredit, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::Destriker>(
      std::bind(&Quoter::OnDestriker, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::VolatilityCurve>(
      std::bind(&Quoter::OnVolatilityCurve, this, std::placeholders::_1));
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
        auto &maturity = inst->Maturity();
        auto it = parameters_.find(maturity);
        if (it == parameters_.end()) {
          auto mp = std::make_shared<MaturityParameter>();
          if (inst->Underlying()->Type() == Proto::InstrumentType::Future) {
            ParameterManager::GetInstance()->GetSSRate(
                inst->Underlying(), maturity, it->second->basis);
          }
          mp->price = dm_->GetUnderlyingTheo();
          it = parameters_.emplace(maturity, mp).first;
        }
        auto param = std::make_shared<Parameter>();
        auto itr = credits.find(op);
        if (itr != credits.end()) {
          param->credit = std::get<0>(itr->second);
          it->second->multiplier = std::get<1>(itr->second);
        }
        ParameterManager::GetInstance()->GetDestriker(inst, param->destriker);
        param->position = PositionManager::GetInstance()->GetNetPosition(inst);
        auto sw = ClusterManager::GetInstance()->FindStrategySwitch(
            Proto::StrategyType::Quoter, inst);
        if (sw) {
          param->is_on = sw->is_bid();
          param->is_qr = sw->is_qr_cover();
        }
        it->second->parameters.emplace(inst, param);
      }
    }

    bid_ = Message<Order>::New();
    bid_->strategy = name_;
    bid_->volume = quoter_->bid_volume();
    bid_->strategy_type = Proto::StrategyType::Quoter;
    bid_->side = Proto::Side::Buy;
    bid_->time_condition = Proto::TimeCondition::GTD;
    bid_->type = Proto::OrderType::Limit;
    bid_->status = Proto::OrderStatus::Local;

    ask_ = Message<Order>::New();
    ask_->strategy = name_;
    ask_->volume = quoter_->ask_volume();
    ask_->strategy_type = Proto::StrategyType::Quoter;
    ask_->side = Proto::Side::Sell;
    ask_->time_condition = Proto::TimeCondition::GTD;
    ask_->type = Proto::OrderType::Limit;
    ask_->status = Proto::OrderStatus::Local;

    orders_.clear();
    order_num_ = 0;
    trade_num_ = 0;
    delta_ = 0;
  } else {
    LOG_ERR << "quoter " << name_ << " isn't existed";
  }
}

void Quoter::OnStop() {
  LOG_INF << "OnStop";
  for (auto &it : parameters_) {
    for (auto &p : it.second->parameters) {
      CancelOrders(p.second);
    }
  }
  PublishStatistic();
}

void Quoter::OnPrice(const PricePtr &price) {
  LOG_DBG << price;
  if (price->instrument->Type() == Proto::InstrumentType::Option) {
    auto it = parameters_.find(price->instrument->Maturity());
    if (it != parameters_.end()) {
      auto itr = it->second->parameters.find(price->instrument);
      if (itr != it->second->parameters.end()) {
        itr->second->price = price;
      }
    }
  } else if (price->instrument == Underlying()) {
    if (unlikely(order_num_ >= quoter_->order_limit())) {
      stop_("order limit broken");
      return;
    }
    for (auto &it : parameters_) {
      it.second->price = price->adjusted_price + it.second->basis;
      // if (unlikely(it.second->multiplier <= 0)) {
      //   LOG_INF << boost::format("credit multiplier of %1% is zero") % it.first;
      //   continue;
      // }
      for (auto &p : it.second->parameters) {
        if (unlikely(p.second->credit <= 0)) {
          LOG_INF << boost::format("credit of %1% is zero") % p.first->Id();
          continue;
        }
        if (unlikely(!p.second->theos)) {
          LOG_INF << boost::format("theo matrix of %1% is null") % p.first->Id();
          continue;
        }
        if (unlikely(p.second->refill_times == quoter_->refill_times())) {
          LOG_INF << boost::format("refill time of %1% is broken") % p.first->Id();
          continue;
        }
        CalculateAndResubmit(p.first, it.second->price, it.second->multiplier, p.second);
      }
    }
  }
}

void Quoter::OnTheoMatrix(const TheoMatrixPtr &theo) {
  LOG_TRA << theo;
  auto it = parameters_.find(theo->option->Maturity());
  if (unlikely(it == parameters_.end())) {
    LOG_DBG << boost::format("can't find maturity %1%") % theo->option->Maturity();
    return;
  }
  auto itr = it->second->parameters.find(theo->option);
  if (unlikely(itr == it->second->parameters.end())) {
    LOG_DBG << "can't find option " << theo->option->Id();
    return;
  }
  itr->second->theos = theo;
  LOG_INF << "update theo matrix of " << theo->option->Id();
  if (unlikely(order_num_ >= quoter_->order_limit())) {
    stop_("order limit broken");
    return;
  }
  // if (unlikely(it->second->multiplier <= 0)) {
  //   LOG_INF << boost::format("credit multiplier of %1% is zero") % it->first;
  //   return;
  // }
  if (unlikely(itr->second->credit <= 0)) {
    LOG_INF << boost::format("credit of %1% is zero") % itr->first->Id();
    return;
  }
  if (unlikely(itr->second->refill_times == quoter_->refill_times())) {
    LOG_INF << boost::format("refill time of %1% is broken") % itr->first->Id();
    return;
  }
  CalculateAndResubmit(theo->option, it->second->price,
                       it->second->multiplier, itr->second);
}

void Quoter::OnOrder(const OrderPtr &order) {
  LOG_DBG << order;
  if (order->strategy == name_ &&
      order->instrument->Type() == Proto::InstrumentType::Option) {
    auto it = parameters_.find(order->instrument->Maturity());
    if (unlikely(it == parameters_.end())) {
      return;
    }
    auto itr = it->second->parameters.find(order->instrument);
    if (unlikely(itr == it->second->parameters.end())) {
      return;
    }
    if (order->IsBid()) {
      auto &bid = itr->second->bid;
      if (bid && bid->id == order->id) {
        LOG_INF << "bid order update: " << order;
        if (order->IsInactive()) {
          if (order->status == Proto::OrderStatus::Rejected) {
            /// stop requote when order is rejected untill replay
            itr->second->refill_times = quoter_->refill_times();
          }
          if (itr->second->ask->IsInactive()) {
            bid.reset();
            itr->second->ask.reset();
            itr->second->canceling = false;
            itr->second->qr_id.clear();
          } else {
            bid = order;
          }
          if (itr->second->is_on &&
              Check(itr->first, it->second->multiplier, itr->second)) {
            ResubmitOrders(itr->first, it->second->multiplier, itr->second);
          }
        } else {
          bid = order;
        }
      }
    } else {
      auto &ask = itr->second->ask;
      if (ask && ask->id == order->id) {
        LOG_INF << "ask order update: " << order;
        if (order->IsInactive()) {
          if (order->status == Proto::OrderStatus::Rejected) {
            /// stop requote when order is rejected untill replay
            itr->second->refill_times = quoter_->refill_times();
          }
          if (itr->second->bid->IsInactive()) {
            itr->second->bid.reset();
            ask.reset();
            itr->second->canceling = false;
            itr->second->qr_id.clear();
          } else {
            ask = order;
          }
          if (itr->second->is_on &&
              Check(itr->first, it->second->multiplier, itr->second)) {
            ResubmitOrders(itr->first, it->second->multiplier, itr->second);
          }
        } else {
          ask = order;
        }
      }
    }
    if (order->status >= Proto::OrderStatus::Canceled) {
      orders_.erase(order->id);
    }
  }
}

void Quoter::OnTrade(const TradePtr &trade) {
  LOG_DBG << trade;
  auto it = orders_.find(trade->order_id);
  if (it != orders_.end()) {
    ++trade_num_;
    if (trade_num_ >= quoter_->trade_limit()) {
      stop_("trade limit broken");
      return;
    }
    delta_ += it->second->delta * trade->volume * trade->instrument->Multiplier() /
      trade->instrument->Underlying()->Multiplier();
    if (base::IsMoreThan(delta_, quoter_->delta_limit())) {
      stop_("delta limit broken");
      return;
    }
  }
  auto it1 = parameters_.find(trade->instrument->Maturity());
  if (it1 != parameters_.end()) {
    auto itr = it1->second->parameters.find(trade->instrument);
    if (itr != it1->second->parameters.end()) {
      itr->second->position = PositionManager::GetInstance()->GetNetPosition(itr->first);
    }
  }
}

bool Quoter::OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat) {
  CheckForAuction();
  CheckForQR();
  PublishStatistic();
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
  auto it = parameters_.find(maturity);
  if (it != parameters_.end()) {
    it->second->price += (msg->rate() - it->second->basis);
    it->second->basis = msg->rate();
    for (auto &p : it->second->parameters) {
      // if (unlikely(p.second->credit <= 0)) {
      //   LOG_INF << boost::format("credit of %1% is zero") % p.first->Id();
      //   continue;
      // }
      // if (unlikely(!p.second->theos)) {
      //   LOG_INF << boost::format("theo matrix of %1% is null") % p.first->Id();
      //   continue;
      // }
      // if (unlikely(p.second->refill_times == quoter_->refill_times())) {
      //   LOG_INF << boost::format("refill time of %1% is broken") % p.first->Id();
      //   continue;
      // }
      // CalculateAndResubmit(p.first, it->second->price, it->second->multiplier, p.second);
      CancelOrders(p.second);
      p.second->theos.reset();
      p.second->theo.Reset();
    }
  }
  // return true;
}

bool Quoter::OnCredit(const std::shared_ptr<Proto::Credit> &msg) {
  LOG_DBG << msg->ShortDebugString();
  auto maturity = boost::gregorian::from_undelimited_string(msg->maturity());
  auto it = parameters_.find(maturity);
  if (it != parameters_.end()) {
    it->second->multiplier = msg->multiplier();
    for (auto &r : msg->records()) {
      auto *inst = InstrumentManager::GetInstance()->FindId(r.option());
      if (inst) {
        auto itr = it->second->parameters.find(inst);
        if (itr != it->second->parameters.end()) {
          itr->second->credit = r.credit();
          if (unlikely(order_num_ >= quoter_->order_limit())) {
            stop_("order limit broken");
            continue;
          }
          if (unlikely(itr->second->refill_times == quoter_->refill_times())) {
            LOG_INF << boost::format("refill time of %1% is broken") % inst->Id();
            continue;
          }
          if (unlikely(!itr->second->theo)) {
            LOG_INF << boost::format("theo of %1% is null") % inst->Id();
            continue;
          }
          CalculateAndResubmit(inst, it->second->price, it->second->multiplier,
                               itr->second);
        }
      }
    }
  }
}

bool Quoter::OnDestriker(const std::shared_ptr<Proto::Destriker> &msg) {
  LOG_INF << "Destriker: " << msg->ShortDebugString();
  auto *inst = InstrumentManager::GetInstance()->FindId(msg->instrument());
  assert (inst);
  auto it = parameters_.find(inst->Maturity());
  if (it != parameters_.end()) {
    auto itr = it->second->parameters.find(inst);
    if (itr != it->second->parameters.end()) {
      itr->second->destriker = msg->destriker();
      if (itr->second->is_on && Check(itr->first, it->second->multiplier, itr->second)) {
        ResubmitOrders(itr->first, it->second->multiplier, itr->second);
      }
    }
  }
}

bool Quoter::OnVolatilityCurve(const std::shared_ptr<Proto::VolatilityCurve> &msg) {
  LOG_DBG << msg->ShortDebugString();
  auto maturity = boost::gregorian::from_undelimited_string(msg->maturity());
  auto it = parameters_.find(maturity);
  if (it != parameters_.end()) {
    for (auto &p : it->second->parameters) {
      CancelOrders(p.second);
      p.second->theos.reset();
      p.second->theo.Reset();
    }
  }
}

// bool Quoter::OnQuoterSpec(const std::shared_ptr<Proto::QuoterSpec> &msg) {
//   LOG_INF << "QuoterSpec: " << msg->ShortDebugString();
// }

bool Quoter::OnStrategySwitch(const std::shared_ptr<Proto::StrategySwitch> &sw) {
  LOG_DBG << sw->ShortDebugString();
  if (sw->strategy() == Proto::StrategyType::Quoter) {
    auto *inst = InstrumentManager::GetInstance()->FindId(sw->option());
    if (inst) {
      auto it = parameters_.find(inst->Maturity());
      if (unlikely(it == parameters_.end())) {
        return false;
      }
      auto itr = it->second->parameters.find(inst);
      if (unlikely(itr == it->second->parameters.end())) {
        return false;
      }
      if (itr->second->is_on != sw->is_bid()) {
        if (itr->second->is_on) {
          /// true --> false
          CancelOrders(itr->second);
        } else if (Check(inst, it->second->multiplier, itr->second)) {
          /// false --> true
          ResubmitOrders(inst, it->second->multiplier, itr->second);
        }
        itr->second->is_on = sw->is_bid();
      }
      itr->second->is_qr = sw->is_qr_cover();
    }
  }
}

bool Quoter::OnStrategyOperate(const std::shared_ptr<Proto::StrategyOperate> &msg) {
  if (msg->name() == Name() && msg->operate() == Proto::StrategyOperation::Start) {
    order_num_ = 0;
    trade_num_ = 0;
    delta_ = 0;
    for (auto &it : parameters_) {
      // if (unlikely(it.second->multiplier <= 0)) {
      //   LOG_INF << boost::format("credit multiplier of %1% is zero") % it.first;
      //   continue;
      // }
      for (auto &p : it.second->parameters) {
        p.second->refill_times = 0;
        if (unlikely(p.second->credit <= 0)) {
          LOG_INF << boost::format("credit of %1% is zero") % p.first->Id();
          continue;
        }
        if (unlikely(!p.second->theo)) {
          LOG_INF << boost::format("theo of %1% is null") % p.first->Id();
          continue;
        }
        CalculateAndResubmit(p.first, it.second->price, it.second->multiplier, p.second);
      }
    }
  }
}

bool Quoter::OnInstrumentReq(const std::shared_ptr<Proto::InstrumentReq> &msg) {
  LOG_INF << "InstrumentReq: " << msg->ShortDebugString();
  for (auto &inst : msg->instruments()) {
    if (inst.type() == Proto::InstrumentType::Option) {
      auto *instrument = InstrumentManager::GetInstance()->FindId(inst.id());
      if (instrument && instrument->HedgeUnderlying () == Underlying()) {
        auto it = parameters_.find(instrument->Maturity());
        if (unlikely(it == parameters_.end())) {
          return false;
        }
        auto itr = it->second->parameters.find(instrument);
        if (unlikely(itr == it->second->parameters.end())) {
          return false;
        }
        auto status = inst.status();
        if (status == Proto::InstrumentStatus::OpeningAuction) {
          CancelOrders(itr->second);
          itr->second->auction_time = base::Now() +
                                      quoter_->open_auction_delay() * base::MILLION;
          itr->second->auction_volume = quoter_->open_auction_volume();
        } else if (status == Proto::InstrumentStatus::ClosingAuction) {
          CancelOrders(itr->second);
          itr->second->auction_time = base::Now() +
                                      quoter_->close_auction_delay() * base::MILLION;
          itr->second->auction_volume = quoter_->close_auction_volume();
        } else if (status == Proto::InstrumentStatus::Fuse) {
          CancelOrders(itr->second);
          itr->second->auction_time = base::Now() +
                                      quoter_->fuse_auction_delay() * base::MILLION;
          itr->second->auction_volume = quoter_->fuse_auction_volume();
        }
        itr->second->status = status;
      }
    }
  }
}

void Quoter::CheckForAuction() {
  auto now = base::Now();
  for (auto &it : parameters_) {
    // if (unlikely(it.second->multiplier <= 0)) {
    //   LOG_INF << boost::format("credit multiplier of %1% is zero") % it.first;
    //   continue;
    // }
    for (auto &p : it.second->parameters) {
      if (Instrument::IsAuction(p.second->status) && p.second->auction_time >= now) {
        if (unlikely(p.second->credit <= 0)) {
          LOG_INF << boost::format("credit of %1% is zero") % p.first->Id();
          continue;
        }
        if (unlikely(!p.second->theo)) {
          LOG_INF << boost::format("theo of %1% is null") % p.first->Id();
          continue;
        }
        if (p.second->theos->FindTheo(it.second->price, p.second->theo)) {
          ResubmitOrders(p.first, it.second->multiplier, p.second);
        } else {
          p.second->theo.Reset();
          LOG_INF << boost::format("can't find theo of %1% by spot %2%") %
                     p.first->Id() % it.second->price;
        }
      }
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
  auto it = parameters_.find(instrument->Maturity());
  if (it != parameters_.end()) {
    auto itr = it->second->parameters.find(instrument);
    if (itr != it->second->parameters.end() && itr->second->is_qr) {
      if (itr->second->bid) {
        CancelOrders(itr->second);
        itr->second->qr_id = rfq->id();
      } else {
        itr->second->qr_id = rfq->id();
        if (Check(itr->first, it->second->multiplier, itr->second)) {
          ResubmitOrders(itr->first, it->second->multiplier, itr->second);
        }
      }
    }
  }
}

void Quoter::PublishStatistic() {
  auto s = Message<Proto::StrategyStatistic>::New();
  s->set_name(name_);
  s->set_type(Proto::StrategyType::Quoter);
  auto *underlying = Underlying();
  s->set_exchange(underlying->Exchange());
  s->set_underlying(underlying->Id());
  s->set_status(Proto::StrategyStatus::Running);
  s->set_delta(delta_);
  s->set_orders(order_num_);
  s->set_trades(trade_num_);
  Middleware::GetInstance()->Publish(s);
}

bool Quoter::Check(const Instrument *inst, double multiplier, ParameterPtr &parameter) {
  if (unlikely(order_num_ >= quoter_->order_limit())) {
    stop_("order limit broken");
    return false;
  }
  if (unlikely(parameter->refill_times == quoter_->refill_times())) {
    LOG_INF << boost::format("refill time of %1% is broken") % inst->Id();
    return false;
  }
  // if (unlikely(multiplier <= 0)) {
  //   LOG_INF << boost::format("credit multiplier of %1% is zero") % inst->Id();
  //   return false;
  // }
  if (unlikely(parameter->credit <= 0)) {
    LOG_INF << boost::format("credit of %1% is zero") % inst->Id();
    return false;
  }
  if (unlikely(!parameter->theo)) {
    LOG_INF << boost::format("theo of %1% is null") % inst->Id();
    return false;
  }
  if (unlikely(parameter->status != Proto::InstrumentStatus::Trading)) {
    LOG_INF << inst->Id() << " is in " << Proto::InstrumentStatus_Name(inst->Status());
    return false;
  }
  return true;
}

void Quoter::CalculateAndResubmit(const Instrument *inst,
                                  double spot,
                                  double multiplier,
                                  ParameterPtr &parameter) {
  if (parameter->theos->FindTheo(spot, parameter->theo)) {
    if (parameter->is_on && parameter->status == Proto::InstrumentStatus::Trading) {
      ResubmitOrders(inst, multiplier, parameter);
    } else {
      CancelOrders(parameter);
    }
  } else {
    CancelOrders(parameter);
    parameter->theo.Reset();
    LOG_INF << boost::format("can't find theo of %1% by spot %2%") % inst->Id() % spot;
  }
}

void Quoter::ResubmitOrders(const Instrument *inst,
                            double multiplier,
                            ParameterPtr &parameter) {
  double theo = parameter->theo.theo + parameter->destriker * parameter->position;
  double bp = inst->RoundToTick(theo - (1 + 0.5 * multiplier) * parameter->credit,
                                Proto::RoundDirection::Down);
  double ap = inst->RoundToTick(theo + (1 + 0.5 * multiplier) * parameter->credit,
                                Proto::RoundDirection::Up);
  double tolerance_ratio = 1;
  if (!quoter_->wide_spread()) {
    double mm_bid = 0, mm_ask = 0;
    if ((parameter->qr_id.empty() && api_->GetMMPrice(inst, theo, mm_bid, mm_ask)) ||
        (!parameter->qr_id.empty() && api_->GetQRPrice(inst, theo, mm_bid, mm_ask))) {
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
  if (base::IsLessThan(bp, inst->Lowest())) {
    if (base::IsLessThan(inst->Lowest(), theo - parameter->credit)) {
      LOG_INF << boost::format("move bid price of %1% from %2% up to %3%") %
                 inst->Id() % bp % inst->Lowest();
      bp = inst->Lowest();
    } else {
      LOG_INF << boost::format("%1%\'s lowest(%2%) >= theo(%3%) - credit(%4%)") %
                 inst->Id() % inst->Lowest() % theo % parameter->credit;
      CancelOrders(parameter);
      return;
    }
  } else if (base::IsMoreThan(bp, inst->Highest())) {
    LOG_INF << boost::format("%1% bid price(%2%) > Highest(%3%)") %
               inst->Id() % bp % inst->Highest();
    CancelOrders(parameter);
    return;
  }
  if (base::IsMoreThan(ap, inst->Highest())) {
    if (base::IsMoreThan(inst->Highest(), theo + parameter->credit)) {
      LOG_INF << boost::format("move ask price of %1% from %2% down to %3%") %
                 inst->Id() % ap % inst->Lowest();
      ap = inst->Highest();
    } else {
      LOG_INF << boost::format("%1%\'s highest(%2%) <= theo(%3%) + credit(%4%)") %
                 inst->Id() % inst->Highest() % theo % parameter->credit;
      CancelOrders(parameter);
      return;
    }
  } else if (base::IsLessThan(ap, inst->Lowest())) {
    LOG_INF << boost::format("%1% ask price(%2%) < Lowest(%3%)") %
               inst->Id() % ap % inst->Lowest();
    CancelOrders(parameter);
    return;
  }
  auto &price = parameter->price;
  if (quoter_->protection() && price) {
    if (price->asks[0] && bp >= price->asks[0].price) {
      LOG_DBG << boost::format("protect mode works(%1%), bid(%2%) >= market ask(%3% %4%)")
                 % inst->Id() % bp % price->asks[0].price % price->asks[0].volume;
      CancelOrders(parameter);
      return;
    }
    if (price->bids[0] && ap <= price->bids[0].price) {
      LOG_DBG << boost::format("protect mode works(%1%), ask(%2%) <= market bid(%3% %4%)")
                 % inst->Id() % ap % price->bids[0].price % price->bids[0].volume;
      CancelOrders(parameter);
      return;
    }
  }
  /// check tolerance
  double tolerance = 0.5 * multiplier * parameter->credit * tolerance_ratio;
  if (parameter->bid &&
      base::IsBetween(parameter->bid->price, bp - tolerance, bp + tolerance) &&
      base::IsBetween(parameter->ask->price, ap - tolerance, ap + tolerance)) {
    LOG_DBG << boost::format("orders in tolerance(%1%): theo(%2%), "
                             "bid(%3%, %4%, %5%), ask(%6%, %7%, %8%)") %
               inst->Id() % theo %
               (bp - tolerance) % parameter->bid->price % (bp + tolerance) %
               (ap - tolerance) % parameter->ask->price % (ap + tolerance);
    return;
  }

  auto build_orders = [&]() {
    bid_->instrument = ask_->instrument = inst;
    bid_->price = bp;
    ask_->price = ap;
    if (parameter->status != Proto::InstrumentStatus::Trading) {
      assert (Instrument::IsAuction(parameter->status));
      bid_->volume = ask_->volume = parameter->auction_volume;
    } else if (unlikely(!parameter->qr_id.empty())){
      bid_->volume = ask_->volume = quoter_->qr_volume();
    }
    bid_->spot = ask_->spot = parameter->theo.spot;
    bid_->volatility = ask_->volatility = parameter->theo.volatility;
    bid_->ss_rate = ask_->ss_rate = parameter->theo.ss_rate;
    bid_->theo = ask_->theo = parameter->theo.theo;
    bid_->delta = ask_->delta = parameter->theo.delta;
    bid_->credit = ask_->credit = parameter->credit;
    parameter->bid = Message<Order>::New(bid_);
    parameter->bid->header.SetTime();
    parameter->bid->ResetId();
    orders_.emplace(parameter->bid->id, parameter->bid);
    PositionManager::GetInstance()->TryFreeze(parameter->bid);
    parameter->ask = Message<Order>::New(ask_);
    parameter->ask->header.SetTime();
    parameter->ask->ResetId();
    orders_.emplace(parameter->ask->id, parameter->ask);
    PositionManager::GetInstance()->TryFreeze(parameter->ask);
    order_num_ += 2;
  };

  if (parameter->bid) {
    assert(parameter->ask);
    if (amend_quote_) {
      build_orders();
      api_->Submit(parameter->bid, parameter->ask);
    } else {
      CancelOrders(parameter);
    }
  } else {
    assert(!parameter->ask);
    build_orders();
    if (quote_) {
      api_->Submit(parameter->bid, parameter->ask);
    } else {
      api_->Submit(parameter->bid);
      api_->Submit(parameter->ask);
    }
  }
}

void Quoter::CancelOrders(const std::shared_ptr<Parameter> &parameter) {
  if (parameter->canceling) {
    LOG_INF << "quote is canceling: bid(" << parameter->bid
            << ") : ask(" <<  parameter->ask << ')';
  } else if (!parameter->qr_id.empty()) {
    LOG_INF << "forbid canceling qr quote: bid(" << parameter->bid
            << ") : ask(" <<  parameter->ask << ')';
  } else if (parameter->bid) {
    assert(parameter->ask);
    if (parameter->status == Proto::InstrumentStatus::Trading) {
      if (quote_) {
        api_->Cancel(parameter->bid, parameter->ask);
      } else {
        if (!parameter->bid->IsInactive()) {
          api_->Cancel(parameter->bid);
          if (!parameter->ask->IsInactive()) {
            api_->Cancel(parameter->ask);
          }
        } else {
          assert(!parameter->ask->IsInactive());
          api_->Cancel(parameter->ask);
        }
      }
      parameter->canceling = true;
    }
  } else {
    assert(!parameter->ask);
  }
}
