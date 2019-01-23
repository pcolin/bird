#ifndef TRADER_API_H
#define TRADER_API_H

#include "WashTradeProtector.h"
#include "model/Order.h"
#include "boost/variant.hpp"
#include "tbb/concurrent_priority_queue.h"
#include <thread>

class TraderApi {
 public:
  TraderApi() : visitor_(this) {}

  virtual void Init();
  virtual void Login() = 0;
  virtual void Logout() = 0;

  void Submit(const OrderPtr& order);
  void Submit(const OrderPtr& bid, const OrderPtr& ask);
  void Amend(const OrderPtr& order);
  void Amend(const OrderPtr& bid, const OrderPtr& ask);
  void Cancel(const OrderPtr& order);
  void Cancel(const OrderPtr& bid, const OrderPtr& ask);
  void CancelAll();

  void OnOrderResponse(const OrderPtr& order);
  void RejectOrder(const OrderPtr& order);

 protected:
  // void StartRequestWork();

  virtual void SubmitOrder(const OrderPtr& order) = 0;
  virtual void SubmitQuote(const OrderPtr& bid, const OrderPtr& ask) = 0;
  virtual void AmendOrder(const OrderPtr& order) = 0;
  virtual void AmendQuote(const OrderPtr& bid, const OrderPtr& ask) = 0;
  virtual void CancelOrder(const OrderPtr& order) = 0;
  virtual void CancelQuote(const OrderPtr& bid, const OrderPtr& ask) = 0;

  virtual void QueryCash() = 0;

  WashTradeProtector protector_;

 private:
  void OnOrderRequest(const OrderRequestPtr& r);
  void OnQuoteRequest(const QuoteRequestPtr& r);
  // void RequestWork();

  class EventVisitor : public boost::static_visitor<void> {
   public:
    EventVisitor(TraderApi* api) : api_(api) {}
    void operator()(const OrderRequestPtr& r) const { api_->OnOrderRequest(r); }
    void operator()(const QuoteRequestPtr& r) const { api_->OnQuoteRequest(r); }

   private:
    TraderApi* api_;
  } visitor_;

  typedef boost::variant<OrderRequestPtr, QuoteRequestPtr> RequestType;
  class RequestComparator {

    class StrategyTypeVisitor : public boost::static_visitor<Proto::StrategyType> {
     public:
      Proto::StrategyType operator()(const OrderRequestPtr& r) const {
        return r->order->strategy_type;
      }
      Proto::StrategyType operator()(const QuoteRequestPtr& r) const {
        return r->bid->strategy_type;
      }
    } visitor_;

   public:
    bool operator() (const RequestType& r1, const RequestType& r2) {
      return boost::apply_visitor(visitor_, r1) > boost::apply_visitor(visitor_, r2);
    }
  };

  std::atomic<bool> running_ = {false};
  std::unique_ptr<std::thread> request_thread_;
  // moodycamel::ConcurrentQueue<RequestType> request_queue_;
  tbb::concurrent_priority_queue<RequestType, RequestComparator> request_queue_;

  std::unique_ptr<std::thread> cash_thread_;
};

#endif // TRADER_API_H
