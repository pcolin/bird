// #include <sys/time.h>
#include "MarketMonitor.h"
#include "Price.pb.h"
#include "Order.pb.h"
#include "Trade.pb.h"
#include "ClusterManager.h"
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"
#include "model/InstrumentManager.h"
#include "model/ParameterManager.h"
#include "model/OrderManager.h"
#include "model/PositionManager.h"
#include "model/Middleware.h"
#include "exchange/manager/ExchangeManager.h"
#include "boost/format.hpp"

MarketMonitor::MarketMonitor(const std::string &name, DeviceManager *dm)
    : Strategy(name, dm),
      orders_(capacity_) {
  dispatcher_.RegisterCallback<Proto::InstrumentReq>(
      std::bind(&MarketMonitor::OnInstrumentReq, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::PriceReq>(
      std::bind(&MarketMonitor::OnPriceReq, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::QuoterSpec>(
      std::bind(&MarketMonitor::OnQuoterSpec, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::Position>(
      std::bind(&MarketMonitor::OnPosition, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::RequestForQuote>(
      std::bind(&MarketMonitor::OnRequestForQuote, this, std::placeholders::_1));

  order_thread_ = std::make_unique<std::thread>(std::bind(&MarketMonitor::RunOrder, this));

  cancel_after_trade_ = EnvConfig::GetInstance()->GetBool(EnvVar::CANCEL_QR_AFTER_TRADE,
      false);
  quote_ = EnvConfig::GetInstance()->GetBool(EnvVar::SUPPORT_SIDE_QUOTE, false) ||
    EnvConfig::GetInstance()->GetBool(EnvVar::SUPPORT_AMEND_QUOTE, false) ||
    EnvConfig::GetInstance()->GetBool(EnvVar::SUPPORT_QUOTE);
}

void MarketMonitor::OnStart() {
  LOG_INF << "OnStart";

  api_ = ExchangeManager::GetInstance()->GetTraderApi();
  assert(api_);
  auto underlyings = InstrumentManager::GetInstance()->FindInstruments(
      [&](const Instrument *inst) {
        return inst->Type() == Proto::Future && inst->HedgeUnderlying() == Underlying();
      });
  for (auto *underlying : underlyings) {
    auto statistic = ClusterManager::GetInstance()->FindStatistic(underlying->Id());
    if (!statistic) {
      statistic = Message<Proto::MarketMakingStatistic>::New();
      statistic->set_underlying(underlying->Id());
      auto product = ParameterManager::GetInstance()->GetProduct(underlying->Product());
      if (product) {
        statistic->set_date(base::DateToString(product->TradingDay()));
      } else {
        statistic->set_date(base::DateToString(boost::gregorian::day_clock::local_day()));
      }
      statistic->set_exchange(underlying->Exchange());
    }
    statistics_[underlying] = statistic;
  }

  auto options = InstrumentManager::GetInstance()->FindInstruments(
      [&](const Instrument *inst) {
        return inst->Type() == Proto::Option && inst->HedgeUnderlying() == Underlying();
      });
  auto now = base::Now();
  for (auto *option : options) {
    quotes_[option].status = option->Status();
    quotes_[option].time = now;
  }

  auto quoters = ClusterManager::GetInstance()->FindQuoters(Underlying());
  if (quoters.size() > 0) {
    qr_timeout_ = quoters[0]->qr_timeout();
  }
}

void MarketMonitor::OnStop() {
  LOG_INF << "OnStop";
}

void MarketMonitor::OnPrice(const PricePtr &price) {
  LOG_DBG << price;
  // struct timeval tv;
  // gettimeofday(&tv, NULL);
  // int now = tv.tv_sec;
  // const int max_interval = 15;
  // int interval = now - opt_price_time_;
  // if (interval >= max_interval) {
  //   LOG_ERR << boost::format("%1% : Option price feed timeout for %2%s") %
  //     dm_->GetUnderlying()->Id() % interval;
  //   opt_price_time_ = now;
  // }
  // interval = now - und_price_time_;
  // if (interval >= max_interval) {
  //   LOG_ERR << boost::format("%1% : Underlying price feed timeout for %2%s") %
  //     dm_->GetUnderlying()->Id() % interval;
  //   und_price_time_ = now;
  // }
  // if (price->instrument->Type() == Proto::InstrumentType::Option) {
  //   opt_price_time_ = now;
  // } else {
  //   und_price_time_ = now;
  // }
  auto p = price->Serialize();
  prices_[price->instrument] = p;
  Middleware::GetInstance()->Publish(p);
}

void MarketMonitor::OnOrder(const OrderPtr &order) {
  // LOG_INF << "OnOrder : " << order->DebugString();
  OrderManager::GetInstance()->OnOrder(order);
  orders_.enqueue(order);

  auto &statistic = statistics_[order->instrument->Underlying()];
  assert(statistic);
  if (order->status == Proto::New || order->status == Proto::Rejected) {
    statistic->set_orders(statistic->orders() + 1);
  }
  if (order->instrument->Type() == Proto::Future && (order->status == Proto::Canceled ||
      order->status == Proto::PartialFilledCanceled)) {
    statistic->set_underlying_cancels(statistic->underlying_cancels() + 1);
  }
  if (order->strategy_type == Proto::StrategyType::Quoter) {
    auto it = quotes_.find(order->instrument);
    assert(it != quotes_.end());

    if (order->status == Proto::Filled &&
        Instrument::IsAuction(it->second.status) &&
        (base::Now() - it->second.time) >= 60 * base::MILLION) {
      AuctionStatistic(it->second, statistic);
    }
    it->second.Update(order);

    if (!order->quote_id.empty()) {
      auto it = qr_orders_.find(order->quote_id);
      if (it != qr_orders_.end()) {
        auto &bid = std::get<0>(it->second);
        auto &ask = std::get<1>(it->second);
        if (order->status == Proto::Submitted) {
          if (order->IsBid()) {
            bid = order;
            if (!ask) {
              std::get<2>(it->second) = base::Now() + qr_timeout_ * base::MILLION;
            }
          } else {
            ask = order;
            if (!bid) {
              std::get<2>(it->second) = base::Now() + qr_timeout_ * base::MILLION;
            }
          }
        } else if (order->status == Proto::PartialFilled) {
          if (cancel_after_trade_) {
            if (quote_) {
              api_->Cancel(bid, ask);
            } else {
              api_->Cancel(bid);
              api_->Cancel(ask);
            }
            statistic->set_valid_qrs(statistic->valid_qrs() + 1);
            qr_orders_.erase(it);
          }
        } else if (order->IsInactive()) {
          if (order->status == Proto::Filled || base::Now() >= std::get<2>(it->second)) {
            statistic->set_valid_qrs(statistic->valid_qrs() + 1);
          }
          if (order->IsBid()) {
            if (!ask->IsInactive()) {
              if (quote_) {
                api_->Cancel(order, ask);
              } else {
                api_->Cancel(ask);
              }
            }
          } else {
            if (!bid->IsInactive()) {
              if (quote_) {
                api_->Cancel(bid, order);
              } else {
                api_->Cancel(bid);
              }
            }
          }
          qr_orders_.erase(it);
        }
      }
    }
  }
}

void MarketMonitor::OnTrade(const TradePtr &trade) {
  // LOG_INF << "OnTrade : " << trade->DebugString();
  Middleware::GetInstance()->Publish(trade->Serialize());
}

bool MarketMonitor::OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat) {
  CancelTimeoutQR();
  QuotingStatistic();
}
// void MarketMonitor::OnLastEvent()
// {
//   if (orders_.size() > 0)
//   {
//     LOG_INF << boost::format("Update %1% orders") % orders_.size();
//     OrderManager::GetInstance()->OnOrder(orders_);
//     orders_.clear();
//   }
// }

bool MarketMonitor::OnInstrumentReq(const std::shared_ptr<Proto::InstrumentReq> &req) {
  LOG_INF << "InstrumentReq: " << req->ShortDebugString();
  // Middleware::GetInstance()->Publish(status);
  auto now = base::Now();
  for (auto &inst : req->instruments()) {
    if (inst.type() == Proto::InstrumentType::Option) {
      auto *instrument = InstrumentManager::GetInstance()->FindId(inst.id());
      if (instrument && instrument->HedgeUnderlying () == Underlying()) {
        auto it = quotes_.find(instrument);
        assert(it != quotes_.end());
        if (it->second.status != inst.status()) {
          auto statistic = statistics_[instrument->Underlying()];
          if (inst.status() == Proto::InstrumentStatus::Fuse) {
            statistic->set_total_fuses(statistic->total_fuses() + 1);
          }
          AuctionStatistic(it->second, statistic);
          it->second.status = inst.status();
          it->second.time = now;
        }
      }
    }
  }
}

bool MarketMonitor::OnRequestForQuote(const std::shared_ptr<Proto::RequestForQuote> &rfq){
  LOG_DBG << rfq->ShortDebugString();
  auto it = qr_orders_.find(rfq->id());
  if (likely(it == qr_orders_.end())) {
    auto expired_time = base::Now() + 60 * base::MILLION;
    qr_orders_.emplace(rfq->id(), std::make_tuple(nullptr, nullptr, expired_time));
  } else {
    LOG_WAN << "receive the same rfq " << rfq->id();
  }
}

bool MarketMonitor::OnPosition(const std::shared_ptr<Proto::Position> &position) {
  LOG_INF << "Position: " << position->ShortDebugString();
  PositionManager::GetInstance()->UpdatePosition(position);
  return true;
}

bool MarketMonitor::OnPriceReq(const std::shared_ptr<Proto::PriceReq> &req) {
  LOG_INF << "PriceReq: " << req->ShortDebugString();
  for (auto it : prices_) {
    Middleware::GetInstance()->Publish(it.second);
  }
  return true;
}

bool MarketMonitor::OnQuoterSpec(const std::shared_ptr<Proto::QuoterSpec> &msg) {
  LOG_INF << "QuoterSpec: " << msg->ShortDebugString();
  qr_timeout_ = msg->qr_timeout();
}

void MarketMonitor::RunOrder() {
  LOG_INF << "Start order publishing thread...";
  OrderPtr orders[capacity_];
  auto req = Message<Proto::OrderReq>::New();
  req->set_type(Proto::RequestType::Set);
  req->set_exchange(Underlying()->Exchange());
  const std::string user = EnvConfig::GetInstance()->GetString(EnvVar::EXCHANGE);
  req->set_user(user);
  std::unordered_map<size_t, int> indexes;
  while (true) {
    size_t cnt = orders_.wait_dequeue_bulk(orders, capacity_);
    for (size_t i = 0; i < cnt; ++i) {
      auto it = indexes.find(orders[i]->id);
      if (it != indexes.end()) {
        if (orders[it->second]->status < orders[i]->status ||
            orders[it->second]->executed_volume < orders[i]->executed_volume) {
          orders[it->second] = orders[i];
        }
        orders[i] = nullptr;
      } else {
        indexes[orders[i]->id] = i;
      }
    }
    for (size_t i = 0; i < cnt; ++i) {
      if (orders[i]) {
        orders[i]->Serialize(req->add_orders());
        if (req->orders_size()  == 10) {
          Middleware::GetInstance()->Publish(req);
          req = Message<Proto::OrderReq>::New();
          req->set_type(Proto::RequestType::Set);
          req->set_exchange(Underlying()->Exchange());
          req->set_user(user);
        }
      }
    }
    if (req->orders_size()) {
      Middleware::GetInstance()->Publish(req);
      req = Message<Proto::OrderReq>::New();
      req->set_type(Proto::RequestType::Set);
      req->set_exchange(Underlying()->Exchange());
      req->set_user(user);
    }
    indexes.clear();
  }
}

void MarketMonitor::QuotingStatistic() {
  for (auto &it : statistics_) {
    it.second->clear_quoting_options();
    it.second->clear_valid_quotes();
    it.second->clear_spread_ratio();
    it.second->clear_warning_options();
  }
  for (auto &it : quotes_) {
    if (it.first->Status() == Proto::Trading && api_->IsMMOption(it.first)) {
      auto &statistic = statistics_[it.first->Underlying()];
      assert(statistic);
      statistic->set_quoting_options(statistic->quoting_options() + 1);
      statistic->set_total_quotes(statistic->total_quotes() + 1);
      auto bid = it.second.GetBestBid();
      auto ask = it.second.GetBestAsk();
      double ratio = 0;
      if (api_->MeetMMObligation(bid, ask, ratio)) {
        statistic->set_valid_quotes(statistic->valid_quotes() + 1);
        statistic->set_cum_valid_quotes(statistic->cum_valid_quotes() + 1);
        statistic->set_spread_ratio(statistic->spread_ratio() + ratio);
        statistic->set_cum_spread_ratio(statistic->cum_spread_ratio() + ratio);
      } else {
        statistic->add_warning_options(it.first->Id());
      }
    }
  }

  for (auto &it : statistics_) {
    Middleware::GetInstance()->Publish(
        Message<Proto::MarketMakingStatistic>::New(*it.second));
  }
}

void MarketMonitor::AuctionStatistic(
    OrderBook &book,
    const std::shared_ptr<Proto::MarketMakingStatistic> &statistic) {
  if (book.status == Proto::OpeningAuction) {
    auto bid = book.GetBestBid();
    auto ask = book.GetBestAsk();
    double ratio = 0;
    if (api_->MeetMMObligation(bid, ask, ratio)) {
      statistic->set_opening_quotes(statistic->opening_quotes() + 1);
    }
  } else if (book.status == Proto::ClosingAuction) {
    auto bid = book.GetBestBid();
    auto ask = book.GetBestAsk();
    double ratio = 0;
    if (api_->MeetMMObligation(bid, ask, ratio)) {
      statistic->set_closing_quotes(statistic->closing_quotes() + 1);
    }
  } else if (Instrument::IsAuction(book.status)) {
    auto bid = book.GetBestBid();
    auto ask = book.GetBestAsk();
    double ratio = 0;
    if (api_->MeetMMObligation(bid, ask, ratio)) {
      statistic->set_fuse_quotes(statistic->fuse_quotes() + 1);
    }
  }
}

void MarketMonitor::CancelTimeoutQR() {
  auto now = base::Now();
  for (auto it = qr_orders_.begin(); it != qr_orders_.end();) {
    if (std::get<2>(it->second) <= now) {
      auto &bid = std::get<0>(it->second);
      auto &ask = std::get<1>(it->second);
      if (bid) {
        assert(!bid->IsInactive() && ask && !ask->IsInactive());
        if (quote_) {
          api_->Cancel(bid, ask);
        } else {
          api_->Cancel(bid);
          api_->Cancel(ask);
        }
        auto &statistic = statistics_[bid->instrument->Underlying()];
        assert(statistic);
        statistic->set_valid_qrs(statistic->valid_qrs() + 1);
      }
      it = qr_orders_.erase(it);
    } else {
      ++it;
    }
  }
}
