#include "TraderApi.h"
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"
#include "model/OrderManager.h"
#include "strategy/base/DeviceManager.h"
#include "strategy/base/ClusterManager.h"

void TraderApi::Init() {
  running_ = true;
  request_thread_ = std::make_unique<std::thread>(
      [&]() {
        LOG_INF << "Start requesting thread...";
        while (running_) {
          RequestType r;
          // if (request_queue_.try_dequeue(r)) {
          if (request_queue_.try_pop(r)) {
            boost::apply_visitor(visitor_, r);
          }
        }
      });

  cash_thread_ = std::make_unique<std::thread>(
      [&]() {
        LOG_INF << "Start querying cash thread...";
        const int interval = EnvConfig::GetInstance()->GetInt32(EnvVar::QRY_CASH_INTERVAL, 15);
        while (running_) {
          QueryCash();
          std::this_thread::sleep_for(std::chrono::seconds(interval));
        }
      });
}

void TraderApi::Submit(const OrderPtr& order) {
  auto r = std::make_shared<OrderRequest>(order, Proto::OrderAction::Submit);
  // request_queue_.enqueue(r);
  request_queue_.push(r);
  LOG_INF << "Submit order: " << order;
}

void TraderApi::Submit(const OrderPtr& bid, const OrderPtr& ask) {
  auto r = std::make_shared<QuoteRequest>(bid, ask, Proto::OrderAction::Submit);
  // request_queue_.enqueue(r);
  request_queue_.push(r);
  LOG_INF << "Submit quote: bid(" << bid << ") ask(" << ask << ')';
}

void TraderApi::Amend(const OrderPtr& order) {
  auto r = std::make_shared<OrderRequest>(order, Proto::OrderAction::Amend);
  // request_queue_.enqueue(r);
  request_queue_.push(r);
  LOG_INF << "Amend order: " << order;
}

void TraderApi::Amend(const OrderPtr& bid, const OrderPtr& ask) {
  auto r = std::make_shared<QuoteRequest>(bid, ask, Proto::OrderAction::Amend);
  // request_queue_.enqueue(r);
  request_queue_.push(r);
  LOG_INF << "Amend quote: bid(" << bid << ") ask(" << ask << ')';
}

void TraderApi::Cancel(const OrderPtr& order) {
  auto r = std::make_shared<OrderRequest>(order, Proto::OrderAction::Cancel);
  // request_queue_.enqueue(r);
  request_queue_.push(r);
  LOG_INF << "Cancel order: " << order;
}

void TraderApi::Cancel(const OrderPtr& bid, const OrderPtr& ask) {
  auto r = std::make_shared<QuoteRequest>(bid, ask, Proto::OrderAction::Cancel);
  // request_queue_.enqueue(r);
  request_queue_.push(r);
  LOG_INF << "Cancel quote: bid(" << bid << ") ask(" << ask << ')';
}

void TraderApi::CancelAll() {
  /// to be done...
}

void TraderApi::OnOrderResponse(const OrderPtr& order) {
  auto* dm = ClusterManager::GetInstance()->FindDevice(order->instrument->HedgeUnderlying());
  if (dm) {
    if (order->IsInactive()) {
      protector_.Remove(order);
    }
    dm->Publish(order);
    // OrderManager::GetInstance()->OnOrder(order);
  } else {
    LOG_ERR << "Can't find device by instrument " << order->instrument->Id();
  }
}

void TraderApi::RejectOrder(const OrderPtr& order) {
  auto ord = std::make_shared<Order>(*order);
  ord->status = Proto::OrderStatus::Rejected;
  OnOrderResponse(ord);
}

// void TraderApi::StartRequestWork()
// {
//   running_ = true;
//   request_thread_ = std::make_unique<std::thread>(std::bind(&TraderApi::RequestWork, this));
// }

void TraderApi::OnOrderRequest(const OrderRequestPtr& r) {
  assert(r);
  switch (r->action) {
    case Proto::OrderAction::Submit: {
      SubmitOrder(r->order);
    break;
    }
    case Proto::OrderAction::Cancel: {
      CancelOrder(r->order);
    break;
    }
    case Proto::OrderAction::Amend: {
      AmendOrder(r->order);
    break;
    }
    default: {
      assert(false);
    }
  }
}

void TraderApi::OnQuoteRequest(const QuoteRequestPtr& r) {
  assert(r);
  switch (r->action) {
    case Proto::OrderAction::Submit: {
      SubmitQuote(r->bid, r->ask);
    break;
    }
    case Proto::OrderAction::Cancel: {
      CancelQuote(r->bid, r->ask);
    break;
    }
    case Proto::OrderAction::Amend: {
      AmendQuote(r->bid, r->ask);
    break;
    }
    default: {
      assert(false);
    }
  }
}
