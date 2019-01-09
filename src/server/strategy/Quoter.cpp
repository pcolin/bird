#include "Quoter.h"
#include <future>
#include "ClusterManager.h"
#include "base/common/Float.h"
#include "base/logger/Logging.h"
#include "exchange/manager/ExchangeManager.h"
#include "model/ProductManager.h"
#include "model/ParameterManager.h"
#include "model/PositionManager.h"
#include "model/Middleware.h"
#include "config/EnvConfig.h"
#include "boost/format.hpp"

Quoter::Quoter(const std::string &name, DeviceManager *dm)
    : Strategy(name, dm) {
  dispatcher_.RegisterCallback<Proto::SSRate>(
      std::bind(&Quoter::OnSSRate, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::Credit>(
      std::bind(&Quoter::OnCredit, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::QuoterSpec>(
      std::bind(&Quoter::OnQuoterSpec, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::StrategySwitch>(
      std::bind(&Quoter::OnStrategySwitch, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::StrategyOperate>(
      std::bind(&Quoter::OnStrategyOperate, this, std::placeholders::_1));

  side_quote_ = EnvConfig::GetInstance()->GetBool(EnvVar::SUPPORT_SIDE_QUOTE, false);
  if (side_quote_) {
    amend_quote_ = quote_ = true;
  } else {
    amend_quote_ = EnvConfig::GetInstance()->GetBool(EnvVar::SUPPORT_AMEND_QUOTE, false);
    if (amend_quote_) {
      quote_ = true;
    } else {
      quote_ = EnvConfig::GetInstance()->GetBool(EnvVar::SUPPORT_QUOTE);
    }
  }
}

void Quoter::OnStart() {
  api_ = ExchangeManager::GetInstance()->GetTraderApi();
  assert(api_);

  quoter_ = ClusterManager::GetInstance()->FindQuoter(name_);
  if (quoter_) {
    std::unordered_map<std::string, std::tuple<double, double>> credits;
    auto tmp = ClusterManager::GetInstance()->FindCredits(
        Proto::StrategyType::Quoter, Underlying());
    for (auto &credit : tmp) {
      for (auto &r : credit->records()) {
        credits[r.option()] = std::make_tuple(r.credit(), credit->multiplier());
      }
    }
    LOG_DBG << boost::format("Get %1% credits") % credits.size();
    parameters_.clear();
    for (auto &op : quoter_->options()) {
      auto *inst = ProductManager::GetInstance()->FindId(op);
      if (inst) {
        auto &maturity = inst->Maturity();
        auto it = parameters_.find(maturity);
        if (it == parameters_.end()) {
          it = parameters_.emplace(maturity, std::make_shared<MaturityParameter>()).first;
        }
        if (inst->Underlying()->Type() == Proto::InstrumentType::Future) {
          ParameterManager::GetInstance()->GetSSRate(
              inst->Underlying(), maturity, it->second->basis);
        }
        auto param = std::make_shared<Parameter>();
        auto itr = credits.find(op);
        if (itr != credits.end()) {
          param->credit = std::get<0>(itr->second);
          it->second->multiplier = std::get<1>(itr->second);
        }
        auto sw = ClusterManager::GetInstance()->FindStrategySwitch(
            Proto::StrategyType::Quoter, inst);
        if (sw) {
          param->is_bid = sw->is_bid();
          param->is_ask = sw->is_ask();
          param->is_qr = sw->is_qr_cover();
        }
        it->second->parameters.emplace(inst, param);
      }
    }

    bid_ = Message::NewOrder();
    bid_->strategy = name_;
    bid_->volume = quoter_->bid_volume();
    bid_->strategy_type = Proto::StrategyType::Quoter;
    bid_->side = Proto::Side::Buy;
    bid_->time_condition = Proto::TimeCondition::GTD;
    bid_->type = Proto::OrderType::Limit;
    bid_->status = Proto::OrderStatus::Local;

    ask_ = Message::NewOrder();
    ask_->strategy = name_;
    ask_->volume = quoter_->ask_volume();
    ask_->strategy_type = Proto::StrategyType::Quoter;
    ask_->side = Proto::Side::Sell;
    ask_->time_condition = Proto::TimeCondition::GTD;
    ask_->type = Proto::OrderType::Limit;
    ask_->status = Proto::OrderStatus::Local;

    order_ids_.clear();
    orders_ = 0;
    trades_ = 0;
    delta_ = 0;
  } else {
    LOG_ERR << "Quoter " << name_ << " isn't existed";
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
    if (unlikely(orders_ >= quoter_->order_limit())) {
      // std::async(std::launch::async, &DeviceManager::Stop, dm_, name_, "order limit broken");
      stop_("order limit broken");
      return;
    }
    for (auto &it : parameters_) {
      it.second->price = price->adjusted_price + it.second->basis;
      if (unlikely(it.second->multiplier <= 0)) {
        LOG_INF << boost::format("Credit multiplier of %1% is zero") % it.first;
        continue;
      }
      for (auto &p : it.second->parameters) {
        if (unlikely(p.second->credit <= 0)) {
          LOG_INF << boost::format("Credit of %1% is zero") % p.first->Id();
          continue;
        }
        if (unlikely(!p.second->theos)) {
          LOG_INF << boost::format("Theo matrix of %1% is null") % p.first->Id();
          continue;
        }
        if (unlikely(p.second->refill_times == quoter_->refill_times())) {
          LOG_INF << boost::format("Refill time of %1% is broken") % p.first->Id();
          continue;
        }
        if (p.second->theos->FindTheo(it.second->price, p.second->theo)) {
          Requote(p.first, it.second->price, it.second->multiplier, p.second);
        } else {
          p.second->theo.spot = base::PRICE_UNDEFINED;
          LOG_INF << boost::format("Can't find theo of %1% by spot %2%") %
            p.first->Id() % it.second->price;
        }
      }
    }
  }
}

void Quoter::OnTheoMatrix(const TheoMatrixPtr &theo) {
  LOG_TRA << theo;
  auto it = parameters_.find(theo->option->Maturity());
  if (it != parameters_.end()) {
    auto itr = it->second->parameters.find(theo->option);
    if (itr != it->second->parameters.end()) {
      itr->second->theos = theo;
      LOG_INF << "Update theo matrix of " << theo->option->Id();
      if (unlikely(orders_ >= quoter_->order_limit())) {
        stop_("order limit broken");
        // std::async(std::launch::async, &DeviceManager::Stop, dm_, name_, "order limit broken");
        return;
      }
      if (unlikely(it->second->multiplier <= 0)) {
        LOG_INF << boost::format("Credit multiplier of %1% is zero") % it->first;
        return;
      }
      if (unlikely(itr->second->credit <= 0)) {
        LOG_INF << boost::format("Credit of %1% is zero") % itr->first->Id();
        return;
      }
      if (unlikely(itr->second->refill_times == quoter_->refill_times())) {
        LOG_INF << boost::format("Refill time of %1% is broken") % itr->first->Id();
        return;
      }
      if (itr->second->theos->FindTheo(it->second->price, itr->second->theo)) {
        Requote(itr->first, it->second->price, it->second->multiplier, itr->second);
      } else {
        itr->second->theo.spot = base::PRICE_UNDEFINED;
        LOG_INF << boost::format("Can't find theo of %1% by spot %2%") %
                   itr->first->Id() % it->second->price;
      }
    } else {
      LOG_DBG << "Can't find option " << theo->option->Id();
    }
  } else {
    LOG_DBG << boost::format("Can't find maturity %1%") % theo->option->Maturity();
  }
}

void Quoter::OnOrder(const OrderPtr &order) {
  LOG_DBG << order;
  if (order->strategy == name_ && order->instrument->Type() == Proto::InstrumentType::Option) {
    auto it = parameters_.find(order->instrument->Maturity());
    if (it != parameters_.end()) {
      auto itr = it->second->parameters.find(order->instrument);
      if (itr != it->second->parameters.end()) {
        if (order->IsBid()) {
          auto &bid = itr->second->bid;
          if (bid && bid->id == order->id) {
            LOG_INF << "Bid order update: " << order;
            bid = order;
            if (order->IsInactive()) {
              itr->second->bid_canceling = false;
              if (order->status == Proto::OrderStatus::Rejected) {
                /// stop requote when order is rejected untill replay
                itr->second->refill_times = quoter_->refill_times();
              } else if (Check(itr->first, it->second->multiplier, itr->second)) {
                Requote(itr->first, it->second->price, it->second->multiplier, itr->second);
              }
            }
          }
        } else {
          auto &ask = itr->second->ask;
          if (ask && ask->id == order->id) {
            LOG_INF << "Ask order update: " << order;
            ask = order;
            if (order->IsInactive()) {
              itr->second->ask_canceling = false;
              if (order->status == Proto::OrderStatus::Rejected) {
                /// stop requote when order is rejected untill replay
                itr->second->refill_times = quoter_->refill_times();
              } else if (Check(itr->first, it->second->multiplier, itr->second)) {
                Requote(itr->first, it->second->price, it->second->multiplier, itr->second);
              }
            }
          }
        }
        if (order->status >= Proto::OrderStatus::Canceled) {
          order_ids_.erase(order->id);
        }
      }
    }
  }
}

void Quoter::OnTrade(const TradePtr &trade) {
  LOG_DBG << trade;
  if (order_ids_.find(trade->order_id) != order_ids_.end()) {
    ++trades_;
    if (trades_ >= quoter_->trade_limit()) {
      // std::async(std::launch::async, &DeviceManager::Stop, dm_, name_, "trade limit broken");
      stop_("trade limit broken");
      return;
    }

    auto it = parameters_.find(trade->instrument->Maturity());
    if (it != parameters_.end()) {
      auto itr = it->second->parameters.find(trade->instrument);
      if (itr != it->second->parameters.end() && itr->second->theo) {
        delta_ += itr->second->theo.delta * trade->volume *
                  trade->instrument->Multiplier() / trade->instrument->Underlying()->Multiplier();
        if (base::IsMoreThan(delta_, quoter_->delta_limit())) {
          stop_("delta limit broken");
          // std::async(std::launch::async, &DeviceManager::Stop, dm_, name_, "delta limit broken");
        }
      }
    }
  }
}

bool Quoter::OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat) {
  PublishStatistic();
}

bool Quoter::OnSSRate(const std::shared_ptr<Proto::SSRate> &msg) {
  LOG_DBG << msg->ShortDebugString();
  auto maturity = boost::gregorian::from_undelimited_string(msg->maturity());
  auto it = parameters_.find(maturity);
  if (it != parameters_.end()) {
    it->second->price += (msg->rate() - it->second->basis);
    it->second->basis = msg->rate();
    if (it->second->multiplier > 0) {
      for (auto &p : it->second->parameters) {
        if (unlikely(p.second->credit <= 0)) {
          LOG_INF << boost::format("Credit of %1% is zero") % p.first->Id();
          continue;
        }
        if (unlikely(!p.second->theos)) {
          LOG_INF << boost::format("Theo matrix of %1% is null") % p.first->Id();
          continue;
        }
        if (unlikely(p.second->refill_times == quoter_->refill_times())) {
          LOG_INF << boost::format("Refill time of %1% is broken") % p.first->Id();
          continue;
        }
        if (p.second->theos->FindTheo(it->second->price, p.second->theo)) {
          Requote(p.first, it->second->price, it->second->multiplier, p.second);
        } else {
          p.second->theo.spot = base::PRICE_UNDEFINED;
          LOG_INF << boost::format("Can't find theo of %1% by spot %2%") %
                     p.first->Id() % it->second->price;
        }
      }
    } else {
      LOG_INF << boost::format("Credit multiplier of %1% is zero") % it->first;
    }
  }
  return true;
}

bool Quoter::OnCredit(const std::shared_ptr<Proto::Credit> &msg) {
  LOG_DBG << msg->ShortDebugString();
  auto maturity = boost::gregorian::from_undelimited_string(msg->maturity());
  auto it = parameters_.find(maturity);
  if (it != parameters_.end()) {
    it->second->multiplier = msg->multiplier();
    for (auto &r : msg->records()) {
      auto *inst = ProductManager::GetInstance()->FindId(r.option());
      if (inst) {
        auto itr = it->second->parameters.find(inst);
        if (itr != it->second->parameters.end()) {
          itr->second->credit = r.credit();
          if (unlikely(orders_ >= quoter_->order_limit())) {
            stop_("order limit broken");
            continue;
          }
          if (unlikely(itr->second->refill_times == quoter_->refill_times())) {
            LOG_INF << boost::format("Refill time of %1% is broken") % inst->Id();
            continue;
          }
          if (unlikely(!itr->second->theo)) {
            LOG_INF << boost::format("Theo of %1% is null") % inst->Id();
            continue;
          }
          Requote(inst, it->second->price, it->second->multiplier, itr->second);
        }
      }
    }
  }
}

bool Quoter::OnQuoterSpec(const std::shared_ptr<Proto::QuoterSpec> &msg) {
  LOG_INF << "QuoterSpec: " << msg->ShortDebugString();
}

bool Quoter::OnStrategySwitch(const std::shared_ptr<Proto::StrategySwitch> &sw) {
  LOG_INF << "StrategySwitch: " << sw->ShortDebugString();
  if (sw->strategy() == Proto::StrategyType::Quoter) {
    auto *inst = ProductManager::GetInstance()->FindId(sw->option());
    if (inst) {
      auto it = parameters_.find(inst->Maturity());
      if (it != parameters_.end()) {
        auto itr = it->second->parameters.find(inst);
        if (itr != it->second->parameters.end()) {
          if (itr->second->is_bid != sw->is_bid()) {
            if (itr->second->is_ask != sw->is_ask()) {
              if (itr->second->is_bid) {
                if (itr->second->is_ask) {
                  /// is_bid: true->false, is_ask: true->false
                  CancelOrders(itr->second);
                  itr->second->is_bid = itr->second->is_ask = false;
                } else {
                  /// is_bid: true->false, is_ask: false->true
                  auto &bid = itr->second->bid;
                  auto &ask = itr->second->ask;
                  if (itr->second->bid_canceling) {
                    LOG_INF << "Bid order is canceling: " << bid;
                  } else if (itr->second->ask_canceling) {
                    LOG_INF << "Ask order is canceling: " << ask;
                    if (!quote_ && bid && !bid->IsInactive()) {
                      api_->Cancel(bid);
                      itr->second->bid_canceling = true;
                    }
                  } else {
                    assert (!ask || ask->IsInactive());
                    if (bid && !bid->IsInactive()) {
                      if (side_quote_) {
                        if (Check(inst, it->second->multiplier, itr->second)) {
                          double theo = itr->second->theo.theo + itr->second->destriker *
                                        itr->second->position;
                          double price = 0;
                          if (GetAskPrice(inst, theo, it->second->multiplier, itr->second->credit,
                                          price)) {
                            bid = nullptr;
                            ask = NewOrder(inst, ask_, price);
                            api_->Submit(nullptr, ask);
                          }
                        }
                      } else {
                        api_->Cancel(bid);
                        itr->second->bid_canceling = true;
                      }
                    } else if (Check(inst, it->second->multiplier, itr->second)) {
                      SubmitAskOrder(inst, it->second->multiplier, itr->second);
                    }
                  }
                  itr->second->is_bid = false;
                  itr->second->is_ask = true;
                }
              } else {
                if (itr->second->is_ask) {
                  /// is_bid: false->true, is_ask: true->false
                  auto &bid = itr->second->bid;
                  auto &ask = itr->second->ask;
                  if (itr->second->ask_canceling) {
                    LOG_INF << "Ask order is canceling: " << ask;
                  } else if (itr->second->bid_canceling) {
                    LOG_INF << "Bid order is canceling: " << bid;
                    if (!quote_ && ask && !ask->IsInactive()) {
                      api_->Cancel(ask);
                      itr->second->ask_canceling = true;
                    }
                  } else {
                    assert (!bid || bid->IsInactive());
                    if (ask && !ask->IsInactive()) {
                      if (side_quote_) {
                        if (Check(inst, it->second->multiplier, itr->second)) {
                          double theo = itr->second->theo.theo + itr->second->destriker *
                                        itr->second->position;
                          double price = 0;
                          if (GetBidPrice(inst, theo, it->second->multiplier, itr->second->credit,
                                          price)) {
                            bid = NewOrder(inst, bid_, price);
                            ask = nullptr;
                            api_->Submit(bid, nullptr);
                          }
                        }
                      } else {
                        api_->Cancel(ask);
                        itr->second->ask_canceling = true;
                      }
                    } else if (Check(inst, it->second->multiplier, itr->second)) {
                      SubmitBidOrder(inst, it->second->multiplier, itr->second);
                    }
                  }
                  itr->second->is_bid = true;
                  itr->second->is_ask = false;
                } else {
                  /// is_bid: false->true, is_ask: false->true
                  if (itr->second->bid_canceling || itr->second->ask_canceling) {
                    LOG_INF << "Quote are canceling: bid(" << itr->second->bid << ") : ask("
                            << itr->second->ask << ')';
                  } else if (Check(inst, it->second->multiplier, itr->second)) {
                    double theo = itr->second->theo.theo + itr->second->destriker *
                                  itr->second->position;
                    SubmitOrders(inst, theo, it->second->multiplier, itr->second);
                  }
                  itr->second->is_bid = itr->second->is_ask = true;
                }
              }
            } else if (itr->second->is_bid) {
              /// is_bid: true->false, is_ask: no change
              if (itr->second->bid_canceling) {
                LOG_INF << "Bid order is canceling: " << itr->second->bid;
              } else if (itr->second->bid && !itr->second->bid->IsInactive()) {
                api_->Cancel(itr->second->bid);
                itr->second->bid_canceling = true;
              }
              itr->second->is_bid = false;
            } else if (itr->second->is_ask) {
              /// is_bid: false->true, is_ask: true->true
              auto &bid = itr->second->bid;
              auto &ask = itr->second->ask;
              if (itr->second->bid_canceling || itr->second->ask_canceling) {
                LOG_INF << "Quote are canceling: bid(" << bid << ") : ask(" << ask << ')';
              } else if (bid) {
                assert (bid->IsInactive());
                if (!ask || ask->IsInactive()) {
                  if (Check(inst, it->second->multiplier, itr->second)) {
                    double theo = itr->second->theo.theo + itr->second->destriker *
                                  itr->second->position;
                    SubmitOrders(inst, theo, it->second->multiplier, itr->second);
                  }
                } else {
                  if (side_quote_) {
                    if (Check(inst, it->second->multiplier, itr->second)) {
                      double theo = itr->second->theo.theo + itr->second->destriker *
                                    itr->second->position;
                      double price = 0;
                      if (GetBidPrice(inst, theo, it->second->multiplier, itr->second->credit,
                                      price)) {
                        bid = NewOrder(inst, bid_, price);
                        if (GetAskPrice(inst, theo, it->second->multiplier, itr->second->credit,
                                        price)) {
                          ask = NewOrder(inst, ask_, price);
                          api_->Submit(bid, ask);
                        } else {
                          ask.reset();
                          api_->Submit(bid, nullptr);
                        }
                      } else if (GetAskPrice(inst, theo, it->second->multiplier,
                                             itr->second->credit, price)) {
                        bid.reset();
                        ask = NewOrder(inst, ask_, price);
                        api_->Submit(nullptr, ask);
                      } else {
                        api_->Cancel(ask);
                        itr->second->ask_canceling = true;
                      }
                    } else {
                      api_->Cancel(ask);
                      itr->second->ask_canceling = true;
                    }
                  } else if (amend_quote_ && Check(inst, it->second->multiplier, itr->second)) {
                    double theo = itr->second->theo.theo + itr->second->destriker *
                                                           itr->second->position;
                    double bp = 0, ap = 0;
                    if (GetBidPrice(inst, theo, it->second->multiplier, itr->second->credit, bp) &&
                        GetAskPrice(inst, theo, it->second->multiplier, itr->second->credit, ap)) {
                      bid = NewOrder(inst, bid_, bp);
                      ask = NewOrder(inst, ask_, ap);
                      api_->Submit(bid, ask);
                    } else {
                      api_->Cancel(ask);
                      itr->second->ask_canceling = true;
                    }
                  } else {
                    api_->Cancel(ask);
                    itr->second->ask_canceling = true;
                  }
                }
              } else if (!ask || ask->IsInactive()) {
                if (Check(inst, it->second->multiplier, itr->second)) {
                  double theo = itr->second->theo.theo + itr->second->destriker *
                                itr->second->position;
                  SubmitOrders(inst, theo, it->second->multiplier, itr->second);
                }
              } else if (side_quote_ && Check(inst, it->second->multiplier, itr->second)) {
                double theo = itr->second->theo.theo + itr->second->destriker *
                              itr->second->position;
                double price = 0;
                if (GetBidPrice(inst, theo, it->second->multiplier, itr->second->credit, price)) {
                  bid = NewOrder(inst, bid_, price);
                  if (GetAskPrice(inst, theo, it->second->multiplier, itr->second->credit, price)) {
                    ask = NewOrder(inst, ask_, price);
                    api_->Submit(bid, ask);
                  } else {
                    ask.reset();
                    api_->Submit(bid, nullptr);
                  }
                } else if(GetAskPrice(inst, theo, it->second->multiplier, itr->second->credit,
                                      price)) {
                  bid.reset();
                  ask = NewOrder(inst, ask_, price);
                  api_->Submit(nullptr, ask);
                } else {
                  api_->Cancel(ask);
                  itr->second->ask_canceling = true;
                }
              } else {
                api_->Cancel(ask);
                itr->second->ask_canceling = true;
              }
              itr->second->is_bid = true;
            } else {
              /// is_bid: false->true, is_ask: false->false
              if (itr->second->bid_canceling || itr->second->ask_canceling) {
                LOG_INF << "Quote are canceling: bid(" << itr->second->bid << ") : ask("
                        << itr->second->ask << ')';
              } else if (Check(inst, it->second->multiplier, itr->second)) {
                double theo = itr->second->theo.theo + itr->second->destriker *
                              itr->second->position;
                double price = 0;
                if (GetBidPrice(inst, theo, it->second->multiplier, itr->second->credit, price)) {
                  itr->second->bid = NewOrder(inst, bid_, price);
                  itr->second->ask = nullptr;
                  if (side_quote_) {
                    api_->Submit(itr->second->bid, nullptr);
                  } else {
                    api_->Submit(itr->second->bid);
                  }
                }
              }
              itr->second->is_bid = true;
            }
          } else if (itr->second->is_ask != sw->is_ask()) {
            if (itr->second->is_bid) {
              if (itr->second->is_ask) {
                /// is_bid: true->true, is_ask: true->false
                auto &ask = itr->second->ask;
                if (itr->second->ask_canceling) {
                  LOG_INF << "Ask order is canceling: " << ask;
                } else if (ask && !ask->IsInactive()) {
                  api_->Cancel(ask);
                  itr->second->ask_canceling = true;
                }
                itr->second->is_ask = false;
              } else {
                /// is_bid: true->true, is_ask: false->true
                auto &bid = itr->second->bid;
                auto &ask = itr->second->ask;
                if (itr->second->bid_canceling || itr->second->ask_canceling) {
                  LOG_INF << "Quote are canceling: bid(" << bid << ") : ask(" << ask << ')';
                } else if (ask) {
                  assert (ask->IsInactive());
                  if (!bid || bid->IsInactive()) {
                    if (Check(inst, it->second->multiplier, itr->second)) {
                      double theo = itr->second->theo.theo + itr->second->destriker *
                                    itr->second->position;
                      SubmitOrders(inst, theo, it->second->multiplier, itr->second);
                    }
                  } else if (side_quote_) {
                    if (Check(inst, it->second->multiplier, itr->second)) {
                      double theo = itr->second->theo.theo + itr->second->destriker *
                                    itr->second->position;
                      double price = 0;
                      if (GetBidPrice(inst, theo, it->second->multiplier, itr->second->credit,
                                      price)) {
                        bid = NewOrder(inst, bid_, price);
                        if (GetAskPrice(inst, theo, it->second->multiplier, itr->second->credit,
                                        price)) {
                          ask = NewOrder(inst, ask_, price);
                          api_->Submit(bid, ask);
                        } else {
                          ask.reset();
                          api_->Submit(bid, nullptr);
                        }
                      } else if (GetAskPrice(inst, theo, it->second->multiplier,
                                             itr->second->credit, price)) {
                        bid.reset();
                        ask = NewOrder(inst, ask_, price);
                        api_->Submit(nullptr, ask);
                      } else {
                        api_->Cancel(bid);
                        itr->second->bid_canceling = true;
                      }
                    } else {
                      api_->Cancel(bid);
                      itr->second->bid_canceling = true;
                    }
                  } else if (amend_quote_ && Check(inst, it->second->multiplier, itr->second)) {
                    double theo = itr->second->theo.theo +
                                  itr->second->destriker * itr->second->position;
                    double bp = 0, ap = 0;
                    if (GetBidPrice(inst, theo, it->second->multiplier, itr->second->credit, bp) &&
                        GetAskPrice(inst, theo, it->second->multiplier, itr->second->credit, ap)) {
                      bid = NewOrder(inst, bid_, bp);
                      ask = NewOrder(inst, ask_, ap);
                      api_->Submit(bid, ask);
                    } else {
                      api_->Cancel(bid);
                      itr->second->bid_canceling = true;
                    }
                  } else {
                    api_->Cancel(bid);
                    itr->second->bid_canceling = true;
                  }
                } else if (!bid || bid->IsInactive()) {
                  if (Check(inst, it->second->multiplier, itr->second)) {
                    double theo = itr->second->theo.theo +
                                  itr->second->destriker * itr->second->position;
                    SubmitOrders(inst, theo, it->second->multiplier, itr->second);
                  }
                } else if (side_quote_ && Check(inst, it->second->multiplier, itr->second)) {
                  double theo = itr->second->theo.theo +
                                itr->second->destriker * itr->second->position;
                  double price = 0;
                  if (GetBidPrice(inst, theo, it->second->multiplier, itr->second->credit, price)) {
                    bid = NewOrder(inst, bid_, price);
                    if (GetAskPrice(inst, theo, it->second->multiplier, itr->second->credit,
                                    price)) {
                      ask = NewOrder(inst, ask_, price);
                      api_->Submit(bid, ask);
                    } else {
                      ask.reset();
                      api_->Submit(bid, nullptr);
                    }
                  } else if (GetAskPrice(inst, theo, it->second->multiplier, itr->second->credit,
                                         price)) {
                    bid.reset();
                    ask = NewOrder(inst, ask_, price);
                    api_->Submit(nullptr, ask);
                  } else {
                    api_->Cancel(bid);
                    itr->second->bid_canceling = true;
                  }
                } else {
                  api_->Cancel(bid);
                  itr->second->bid_canceling = true;
                }
                itr->second->is_ask = true;
              }
            } else {
              if (itr->second->is_ask) {
                /// is_bid: false->false, is_ask: true->false
                auto &ask = itr->second->ask;
                if (itr->second->ask_canceling) {
                  LOG_INF << "Ask order is canceling: " << ask;
                } else if (ask && !ask->IsInactive()) {
                  api_->Cancel(ask);
                  itr->second->ask_canceling = true;
                }
                itr->second->is_ask = false;
              } else {
                /// is_bid: false->false, is_ask: false->true
                if (itr->second->bid_canceling || itr->second->ask_canceling) {
                  LOG_INF << "Quote are canceling: bid(" << itr->second->bid << ") : ask("
                          << itr->second->ask << ')';
                } else if (Check(inst, it->second->multiplier, itr->second)) {
                  double theo = itr->second->theo.theo +
                                itr->second->destriker * itr->second->position;
                  double price = 0;
                  if (GetAskPrice(inst, theo, it->second->multiplier, itr->second->credit, price)) {
                    itr->second->bid = nullptr;
                    itr->second->ask = NewOrder(inst, ask_, price);
                    if (side_quote_) {
                      api_->Submit(nullptr, itr->second->ask);
                    } else {
                      api_->Submit(itr->second->ask);
                    }
                  }
                }
                itr->second->is_ask = true;
              }
            }
          }
          itr->second->is_qr = sw->is_qr_cover();
        }
      }
    }
  }
}

bool Quoter::OnStrategyOperate(const std::shared_ptr<Proto::StrategyOperate> &msg) {
  if (msg->name() == Name() && msg->operate() == Proto::StrategyOperation::Start) {
    orders_ = 0;
    trades_ = 0;
    delta_ = 0;
    for (auto &it : parameters_) {
      if (unlikely(it.second->multiplier <= 0)) {
        LOG_INF << boost::format("Credit multiplier of %1% is zero") % it.first;
        continue;
      }
      for (auto &p : it.second->parameters) {
        p.second->refill_times = 0;
        if (unlikely(p.second->credit <= 0)) {
          LOG_INF << boost::format("Credit of %1% is zero") % p.first->Id();
          continue;
        }
        if (unlikely(!p.second->theo)) {
          LOG_INF << boost::format("Theo of %1% is null") % p.first->Id();
          continue;
        }
        Requote(p.first, it.second->price, it.second->multiplier, p.second);
      }
    }
  }
}

void Quoter::PublishStatistic() {
  auto s = Message::NewProto<Proto::StrategyStatistic>();
  s->set_name(name_);
  s->set_type(Proto::StrategyType::Quoter);
  auto *underlying = Underlying();
  s->set_exchange(underlying->Exchange());
  s->set_underlying(underlying->Id());
  s->set_status(Proto::StrategyStatus::Running);
  s->set_delta(delta_);
  s->set_orders(orders_);
  s->set_trades(trades_);
  Middleware::GetInstance()->Publish(s);
}

bool Quoter::Check(const Instrument *inst, double multiplier, ParameterPtr &parameter) {
  if (unlikely(orders_ >= quoter_->order_limit())) {
    // std::async(std::launch::async, &DeviceManager::Stop, dm_, name_, "order limit broken");
    stop_("order limit broken");
    return false;
  }
  if (unlikely(parameter->refill_times == quoter_->refill_times())) {
    LOG_INF << boost::format("Refill time of %1% is broken") % inst->Id();
    return false;
  }
  if (unlikely(multiplier <= 0)) {
    LOG_INF << boost::format("Credit multiplier of %1% is zero") % inst->Id();
    return false;
  }
  if (unlikely(parameter->credit <= 0)) {
    LOG_INF << boost::format("Credit of %1% is zero") % inst->Id();
    return false;
  }
  if (unlikely(!parameter->theo)) {
    LOG_INF << boost::format("Theo of %1% is null") % inst->Id();
    return false;
  }
  return true;
}

void Quoter::Requote(const Instrument *inst,
                     double spot,
                     double multiplier,
                     std::shared_ptr<Parameter> &parameter) {
  auto &bid = parameter->bid;
  auto &ask = parameter->ask;
  if (parameter->is_bid) {
    if (parameter->is_ask) {
      if (bid) {
        if (bid->IsInactive()) {
          if (!ask || ask->IsInactive()) {
            /// is_bid=true, is_ask=true, bid=inactive, ask=null or inactive
            assert(!parameter->bid_canceling);
            assert(!parameter->ask_canceling);
            double theo = parameter->theo.theo + parameter->destriker * parameter->position;
            SubmitOrders(inst, theo, multiplier, parameter);
          } else {
            /// is_bid=true, is_ask=true, bid=inactive, ask=active
            if (parameter->ask_canceling) {
              LOG_INF << "Ask order is canceling: " << ask;
            } else {
              double theo = parameter->theo.theo + parameter->destriker * parameter->position;
              if (IsAskAdjusted(ask, theo, multiplier, parameter->credit)) {
                if (amend_quote_) {
                  SubmitOrders(inst, theo, multiplier, parameter);
                } else if (quote_) {
                  api_->Cancel(bid, ask);
                  parameter->ask_canceling = true;
                } else {
                  api_->Cancel(ask);
                  parameter->ask_canceling = true;
                }
              } else {
                LOG_DBG << inst->Id() << " needn't adjust ask: " << ask;
              }
            }
          }
        } else if (ask) {
          if (ask->IsInactive()) {
            /// is_bid=true, is_ask=true, bid=active, ask=inactive
            if (parameter->bid_canceling) {
              LOG_INF << "Bid order is canceling: " << bid;
            } else {
              double theo = parameter->theo.theo + parameter->destriker * parameter->position;
              if (IsBidAdjusted(bid, theo, multiplier, parameter->credit)) {
                if (amend_quote_) {
                  SubmitOrders(inst, theo, multiplier, parameter);
                } else {
                  api_->Cancel(bid);
                  parameter->bid_canceling = true;
                }
              } else {
                LOG_DBG << inst->Id() << " needn't adjust bid: " << bid;
              }
            }
          } else {
            /// is_bid=true, is_ask=true, bid=active, ask=active
            if (parameter->bid_canceling || parameter->ask_canceling) {
              LOG_INF << "Quote is canceling: bid(" << bid << ") ask(" << ask << ')';
            } else {
              double theo = parameter->theo.theo + parameter->destriker * parameter->position;
              if (IsBidAdjusted(bid, theo, multiplier, parameter->credit) ||
                  IsAskAdjusted(ask, theo, multiplier, parameter->credit)) {
                if (amend_quote_) {
                  SubmitOrders(inst, theo, multiplier, parameter);
                } else if (quote_) {
                  api_->Cancel(bid, ask);
                  parameter->bid_canceling = true;
                  parameter->ask_canceling = true;
                } else {
                  api_->Cancel(bid);
                  api_->Cancel(ask);
                  parameter->bid_canceling = true;
                  parameter->ask_canceling = true;
                }
              } else {
                LOG_DBG << inst->Id() << " needn't adjust quote: bid("
                        << bid << ") ask(" << ask << ')';
              }
            }
          }
        } else {
          /// is_bid=true, is_ask=true, bid=active, ask=null
          if (parameter->bid_canceling) {
            LOG_INF << "Bid order is canceling: " << bid;
          } else {
            double theo = parameter->theo.theo + parameter->destriker * parameter->position;
            if (IsBidAdjusted(bid, theo, multiplier, parameter->credit)) {
              if (side_quote_) {
                SubmitOrders(inst, theo, multiplier, parameter);
              } else {
                api_->Cancel(bid);
                parameter->bid_canceling = true;
              }
            } else {
              LOG_DBG << inst->Id() << " needn't adjust bid: " << bid;
            }
          }
        }
      } else if (!ask || ask->IsInactive()) {
        /// is_bid=true, is_ask=true, bid=null, ask=null or inactive
        assert(!parameter->bid_canceling);
        assert(!parameter->ask_canceling);
        double theo = parameter->theo.theo + parameter->destriker * parameter->position;
        SubmitOrders(inst, theo, multiplier, parameter);
      } else {
        /// is_bid=true, is_ask=true, bid=null, ask=active
        if (parameter->ask_canceling) {
          LOG_INF << "Ask order is canceling: " << ask;
        } else {
          double theo = parameter->theo.theo + parameter->destriker * parameter->position;
          if (IsAskAdjusted(ask, theo, multiplier, parameter->credit)) {
            if (side_quote_) {
              SubmitOrders(inst, theo, multiplier, parameter);
            } else {
              api_->Cancel(ask);
              parameter->ask_canceling = true;
            }
          } else {
            LOG_DBG << inst->Id() << " needn't adjust ask: " << ask;
          }
        }
      }
    } else if (bid) {
      if (bid->IsInactive()) {
        if (!ask || ask->IsInactive()) {
          /// is_bid=true, is_ask=false, bid=inactive, ask=null or inactive
          SubmitBidOrder(inst, multiplier, parameter);
        } else {
          /// is_bid=true, is_ask=false, bid=inactive, ask=active
          assert (parameter->ask_canceling);
        }
      } else if (!ask || ask->IsInactive()) {
        /// is_bid=true, is_ask=false, bid=active, ask=null or inactive
        ResubmitBidOrder(inst, multiplier, parameter);
      } else {
        /// is_bid=true, is_ask=false, bid=active, ask=active
        assert (parameter->ask_canceling);
      }
    } else if (!ask || ask->IsInactive()) {
      /// is_bid=true, is_ask=false, bid=null, ask=null or inactive
      SubmitBidOrder(inst, multiplier, parameter);
    } else {
      /// is_bid=true, is_ask=false, bid=null, ask=active
      assert (parameter->ask_canceling);
    }
  } else if (parameter->is_ask) {
    if (bid) {
      if (bid->IsInactive()) {
        if (!ask || ask->IsInactive()) {
          /// is_bid=false, is_ask=true, bid=inactive, ask=null or inactive
          SubmitAskOrder(inst, multiplier, parameter);
        } else {
          /// is_bid=false, is_ask=true, bid=inactive, ask=active
          ResubmitAskOrder(inst, multiplier, parameter);
        }
      } else {
        /// is_bid=false, is_ask=true, bid=active, ask=null or inactive or active
        assert (parameter->bid_canceling);
      }
    } else if (!ask || ask->IsInactive()) {
      /// is_bid=false, is_ask=true, bid=null, ask=null or inactive
      SubmitAskOrder(inst, multiplier, parameter);
    } else {
      /// is_bid=false, is_ask=true, bid=null, ask=active
      ResubmitAskOrder(inst, multiplier, parameter);
    }
  }
}

bool Quoter::GetBidPrice(const Instrument *inst,
                         double theo,
                         double multiplier,
                         double credit,
                         double &price) {
  price = inst->RoundToTick(theo - (1 + 0.5 * multiplier) * credit, Proto::RoundDirection::Down);
  if (base::IsLessThan(price, inst->Lowest())) {
    if (base::IsLessThan(inst->Lowest(), theo - credit)) {
      LOG_INF << boost::format("Move bid price of %1% up to %2%") % inst->Id() % inst->Lowest();
      price = inst->Lowest();
      return true;
    } else {
      LOG_INF << boost::format("%1%\'s lowest(%2%) >= theo(%3%) - credit(%4%)") %
        inst->Id() % inst->Lowest() % theo % credit;
      return false;
    }
  } else if (base::IsMoreThan(price, inst->Highest())) {
    LOG_INF << boost::format("%1% bid price(%2%) > Highest(%3%)") %
               inst->Id() % price % inst->Highest();
    return false;
  }
  return true;
}

bool Quoter::GetAskPrice(const Instrument *inst,
                         double theo,
                         double multiplier,
                         double credit,
                         double &price) {
  price = inst->RoundToTick(theo + (1 + 0.5 * multiplier) * credit, Proto::RoundDirection::Up);
  if (base::IsMoreThan(price, inst->Highest())) {
    if (base::IsMoreThan(inst->Highest(), theo + credit)) {
      LOG_INF << boost::format("Move ask price of %1% down to %2%") % inst->Id() % inst->Lowest();
      price = inst->Highest();
      return true;
    } else {
      LOG_INF << boost::format("%1%\'s highest(%2%) <= theo(%3%) + credit(%4%)") %
                 inst->Id() % inst->Highest() % theo % credit;
      return false;
    }
  } else if (base::IsLessThan(price, inst->Lowest())) {
    LOG_INF << boost::format("%1% ask price(%2%) < Lowest(%3%)") %
               inst->Id() % price % inst->Lowest();
    return false;
  }
  return true;
}

// bool Quoter::GetBidAskPrice(const Instrument *inst, double theo, double multiplier, double credit,
//     double &bid_price, double &ask_price)
// {
//   if (GetBidPrice(inst, theo, multiplier, credit, bid_price))
//   {

//   }
//   return false;

//   if (base::IsLessThan(bid_price, inst->Highest()))
//   {
//     ask_price = inst->RoundToTick(theo + (1 + 0.5*multiplier) * credit, Proto::RoundDirection::Up);
//     if (base::IsMoreThan(ask_price, inst->Highest()))
//     {
//       if (base::IsMoreThan(inst->Highest(), theo + credit))
//       {
//         LOG_INF << boost::format("Move ask price of %1% down to %2%") % inst->Id() % inst->Lowest();
//         ask_price = inst->Highest();
//       }
//       else
//       {
//         LOG_INF << boost::format("%1%\'s highest(%2%) <= theo(%3%) + credit(%4%)") %
//           inst->Id() % inst->Highest() % theo % credit;
//         ask_price = 0;
//       }
//     }
//     else if (base::IsMoreThan(ask_price, inst->Lowest()))
//     {
//       return true;
//     }
//     else
//     {
//       LOG_INF << boost::format("ask price(%1%) of %2% <= lowest(%3%)") %
//         inst->Id() % ask_price % inst->Lowest();
//       return false;
//     }
//   }
//   else
//   {
//     LOG_INF << boost::format("bid price(%1%) of %2% >= highest(%3%)") %
//       inst->Id() % bid_price % inst->Highest();
//     return false;
//   }
// }

bool Quoter::IsBidAdjusted(const OrderPtr &bid, double theo, double multiplier, double credit) {
  if (bid->IsInactive()) {
    return true;
  }
  double max = bid->instrument->RoundToTick(theo - credit, Proto::RoundDirection::Down);
  double min = bid->instrument->RoundToTick(max - multiplier * credit, Proto::RoundDirection::Down);
  return base::IsLessThan(bid->price, min) || base::IsMoreThan(bid->price, max);
}

bool Quoter::IsAskAdjusted(const OrderPtr &ask, double theo, double multiplier, double credit) {
  if (ask->IsInactive()) {
    return true;
  }
  double min = ask->instrument->RoundToTick(theo + credit, Proto::RoundDirection::Up);
  double max = ask->instrument->RoundToTick(min + multiplier * credit, Proto::RoundDirection::Up);
  return base::IsLessThan(ask->price, min) || base::IsMoreThan(ask->price, max);
}

void Quoter::SubmitBidOrder(const Instrument *inst, double multiplier, ParameterPtr &parameter) {
  double theo = parameter->theo.theo + parameter->destriker * parameter->position;
  double price = 0;
  if (GetBidPrice(inst, theo, multiplier, parameter->credit, price)) {
    parameter->bid = NewOrder(inst, bid_, price);
    if (side_quote_) {
      api_->Submit(parameter->bid, nullptr);
    } else {
      api_->Submit(parameter->bid);
    }
  }
}

void Quoter::ResubmitBidOrder(const Instrument *inst, double multiplier, ParameterPtr &parameter) {
  if (parameter->bid_canceling) {
    LOG_INF << "Bid order is canceling: " << parameter->bid;
  } else {
    double theo = parameter->theo.theo + parameter->destriker * parameter->position;
    if (IsBidAdjusted(parameter->bid, theo, multiplier, parameter->credit)) {
      if (side_quote_) {
        double price = 0;
        if (GetBidPrice(inst, theo, multiplier, parameter->credit, price)) {
          parameter->bid = NewOrder(inst, bid_, price);
          parameter->ask = nullptr;
          api_->Submit(parameter->bid, nullptr);
        }
      } else {
        api_->Cancel(parameter->bid);
        parameter->bid_canceling = true;
      }
    } else {
      LOG_DBG << inst->Id() << " needn't adjust bid: " << parameter->bid;
    }
  }
}

void Quoter::SubmitAskOrder(const Instrument *inst, double multiplier, ParameterPtr &parameter) {
  double theo = parameter->theo.theo + parameter->destriker * parameter->position;
  double price = 0;
  if (GetAskPrice(inst, theo, multiplier, parameter->credit, price)) {
    parameter->ask = NewOrder(inst, ask_, price);
    if (side_quote_) {
      api_->Submit(nullptr, parameter->ask);
    } else {
      api_->Submit(parameter->ask);
    }
  }
}

void Quoter::ResubmitAskOrder(const Instrument *inst, double multiplier, ParameterPtr &parameter) {
  if (parameter->ask_canceling) {
    LOG_INF << "Ask order is canceling: " << parameter->ask;
  } else {
    double theo = parameter->theo.theo + parameter->destriker * parameter->position;
    if (IsAskAdjusted(parameter->ask, theo, multiplier, parameter->credit)) {
      if (side_quote_) {
        double price = 0;
        if (GetAskPrice(inst, theo, multiplier, parameter->credit, price)) {
          parameter->bid = nullptr;
          parameter->ask = NewOrder(inst, ask_, price);
          api_->Submit(nullptr, parameter->ask);
        }
      } else {
        api_->Cancel(parameter->ask);
        parameter->ask_canceling = true;
      }
    } else {
      LOG_DBG << inst->Id() << " needn't adjust ask: " << parameter->ask;
    }
  }
}

void Quoter::SubmitOrders(const Instrument *inst,
                          double theo,
                          double multiplier,
                          ParameterPtr &parameter) {
  auto &bid = parameter->bid;
  auto &ask = parameter->ask;
  double price = 0;
  if (GetBidPrice(inst, theo, multiplier, parameter->credit, price)) {
    bid = NewOrder(inst, bid_, price);
    if (GetAskPrice(inst, theo, multiplier, parameter->credit, price)) {
      ask = NewOrder(inst, ask_, price);
      if (quote_) {
        api_->Submit(bid, ask);
      } else {
        api_->Submit(bid);
        api_->Submit(ask);
      }
    } else {
      ask.reset();
      if (side_quote_) {
        api_->Submit(bid, nullptr);
      } else {
        api_->Submit(bid);
      }
    }
  } else if (GetAskPrice(inst, theo, multiplier, parameter->credit, price)) {
    bid.reset();
    ask = NewOrder(inst, ask_, price);
    if (side_quote_) {
      api_->Submit(nullptr, ask);
    } else {
      api_->Submit(ask);
    }
  }
}

void Quoter::CancelOrders(const std::shared_ptr<Parameter> &parameter) {
  auto &bid = parameter->bid;
  auto &ask = parameter->ask;
  if (bid) {
    if (ask) {
      if (quote_) {
        if (parameter->bid_canceling || parameter->ask_canceling) {
          LOG_INF << "Quote are canceling: bid(" << bid << ") : ask(" <<  ask << ')';
        } else if (!bid->IsInactive() ||!ask->IsInactive()) {
          api_->Cancel(bid, ask);
        }
      } else {
        if (parameter->bid_canceling) {
          LOG_INF << "Bid order is canceling: " << bid;
        } else if (!bid->IsInactive()) {
          api_->Cancel(bid);
        }

        if (parameter->ask_canceling) {
          LOG_INF << "Ask order is canceling: " << ask;
        } else if (!ask->IsInactive()) {
          api_->Cancel(ask);
        }
      }
    } else {
      if (parameter->bid_canceling) {
        LOG_INF << "Bid order is canceling: " << bid;
      } else if (!bid->IsInactive()) {
        api_->Cancel(bid);
      }
    }
  } else {
    if (parameter->ask_canceling) {
      LOG_INF << "Ask order is canceling: " << ask;
    } else if (ask && !ask->IsInactive()) {
      api_->Cancel(ask);
    }
  }
}

OrderPtr Quoter::NewOrder(const Instrument *inst, const OrderPtr &order, base::PriceType price) {
  auto ord = Message::NewOrder(order);
  ord->header.SetTime();
  ord->ResetId();
  order_ids_.insert(ord->id);
  ++orders_;
  ord->instrument = inst;
  ord->price = price;
  PositionManager::GetInstance()->TryFreeze(ord);
  return ord;
}
