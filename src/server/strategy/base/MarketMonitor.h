#ifndef STRATEGY_MARKET_MONITOR_H
#define STRATEGY_MARKET_MONITOR_H

#include "base/concurrency/blockingconcurrentqueue.h"
#include "Strategy.h"
#include "Position.pb.h"
#include "RequestForQuote.pb.h"
#include "MarketMakingStatistic.pb.h"

class TraderApi;
class MarketMonitor : public Strategy {
  class OrderBook {
   public:
    void Update(const OrderPtr &order) {
      if (order->IsBid()) {
        if (order->IsInactive()) {
          bids.remove_if([&](const OrderPtr& ord) { return order->id == ord->id; });
        } else {
          auto it = bids.begin();
          while (it != bids.end()) {
            if ((*it)->price < order->price) {
              bids.insert(it, order);
              break;
            } else if ((*it)->price == order->price && (*it)->id == order->id) {
              *it = order;
              break;
            }
            ++it;
          }
        }
      } else {
        if (order->IsInactive()) {
          asks.remove_if([&](const OrderPtr& ord) { return order->id == ord->id; });
        } else {
          auto it = asks.begin();
          while (it != asks.end()) {
            if ((*it)->price > order->price) {
              asks.insert(it, order);
              break;
            } else if ((*it)->price == order->price && (*it)->id == order->id) {
              *it = order;
              break;
            }
            ++it;
          }
        }
      }
    }

    OrderPtr GetBestBid() const { return (bids.size() > 0) ? bids.front() : nullptr; }
    OrderPtr GetBestAsk() const { return (asks.size() > 0) ? bids.front() : nullptr; }

    Proto::InstrumentStatus status;
    int64_t time;

   private:
    std::list<OrderPtr> bids;
    std::list<OrderPtr> asks;
  };
  typedef std::unordered_map<const Instrument*, OrderBook> OrderBookMap;

  typedef std::unordered_map<const Instrument*, std::shared_ptr<Proto::Price>> PriceMap;
 public:
  MarketMonitor(const std::string &name, DeviceManager *dm);

  virtual void OnStart() override;
  virtual void OnStop() override;

 protected:
  virtual void OnPrice(const PricePtr &price) override;
  virtual void OnOrder(const OrderPtr &order) override;
  virtual void OnTrade(const TradePtr &trade) override;
  virtual bool OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat) override;
  // virtual void OnLastEvent() override;

 private:
  bool OnInstrumentReq(const std::shared_ptr<Proto::InstrumentReq> &req);
  bool OnRequestForQuote(const std::shared_ptr<Proto::RequestForQuote> &rfq);
  bool OnPosition(const std::shared_ptr<Proto::Position> &position);
  bool OnPriceReq(const std::shared_ptr<Proto::PriceReq> &req);
  bool OnQuoterSpec(const std::shared_ptr<Proto::QuoterSpec> &msg);
  // int und_price_time_;
  // int opt_price_time_;

  void RunOrder();
  void QuotingStatistic();
  void AuctionStatistic(
      OrderBook &book,
      const std::shared_ptr<Proto::MarketMakingStatistic> &statistic);
  void CancelTimeoutQR();

  TraderApi *api_ = nullptr;

  static const size_t capacity_ = 1024;
  moodycamel::BlockingConcurrentQueue<OrderPtr> orders_;
  std::unique_ptr<std::thread> order_thread_;

  OrderBookMap quotes_;
  std::unordered_map<std::string, std::tuple<OrderPtr, OrderPtr, int64_t>> qr_orders_;
  int32_t qr_timeout_;
  bool cancel_after_trade_;
  bool quote_;

  PriceMap prices_;
  // std::unique_ptr<std::thread> cash_thread_;
  std::unordered_map<const Instrument*,
                     std::shared_ptr<Proto::MarketMakingStatistic>> statistics_;
};

#endif // STRATEGY_MARKET_MONITOR_H
