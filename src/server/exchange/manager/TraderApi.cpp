#include "TraderApi.h"
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"
#include "model/OrderManager.h"
#include "strategy/DeviceManager.h"
#include "strategy/ClusterManager.h"

void TraderApi::Init()
{
  running_ = true;
  request_thread_ = std::make_unique<std::thread>([&]()
    {
      LOG_INF << "Start requesting thread...";
      while (running_)
      {
        RequestType r;
        if (request_queue_.try_dequeue(r))
        {
          boost::apply_visitor(visitor_, r);
        }
      }
    });

  cash_thread_ = std::make_unique<std::thread>([&]()
    {
      LOG_INF << "Start querying cash thread...";
      const int interval = EnvConfig::GetInstance()->GetInt32(EnvVar::QRY_CASH_INTERVAL, 15);
      while (running_)
      {
        QueryCash();
        std::this_thread::sleep_for(std::chrono::seconds(interval));
      }
    });
}

void TraderApi::New(const OrderPtr &order)
{
  auto r = std::make_shared<OrderRequest>(order, OrderAction::New);
  request_queue_.enqueue(r);
}

void TraderApi::New(const OrderPtr &bid, const OrderPtr &ask)
{
  auto r = std::make_shared<QuoteRequest>(bid, ask, OrderAction::New);
  request_queue_.enqueue(r);
}

void TraderApi::Amend(const OrderPtr &order)
{
  auto r = std::make_shared<OrderRequest>(order, OrderAction::Amend);
  request_queue_.enqueue(r);
}

void TraderApi::Amend(const OrderPtr &bid, const OrderPtr &ask)
{
  auto r = std::make_shared<QuoteRequest>(bid, ask, OrderAction::Amend);
  request_queue_.enqueue(r);
}

void TraderApi::Pull(const OrderPtr &order)
{
  auto r = std::make_shared<OrderRequest>(order, OrderAction::Pull);
  request_queue_.enqueue(r);
}

void TraderApi::Pull(const OrderPtr &bid, const OrderPtr &ask)
{
  auto r = std::make_shared<QuoteRequest>(bid, ask, OrderAction::Pull);
  request_queue_.enqueue(r);
}

void TraderApi::PullAll()
{
  /// to be done...
}

void TraderApi::OnOrderResponse(const OrderPtr &order)
{
  auto *dm = ClusterManager::GetInstance()->FindDevice(order->instrument->HedgeUnderlying());
  if (dm)
  {
    dm->Publish(order);
    // OrderManager::GetInstance()->OnOrder(order);
  }
  else
  {
    LOG_ERR << "Can't find device by instrument " << order->instrument->Id();
  }
}

void TraderApi::RejectOrder(const OrderPtr &order)
{
  auto ord = Message::NewOrder(order);
  ord->status = OrderStatus::Rejected;
  OnOrderResponse(ord);
}

// void TraderApi::StartRequestWork()
// {
//   running_ = true;
//   request_thread_ = std::make_unique<std::thread>(std::bind(&TraderApi::RequestWork, this));
// }

void TraderApi::OnOrderRequest(const OrderRequestPtr &r)
{
  assert(r);
  switch (r->action)
  {
    case OrderAction::New:
      NewOrder(r->order);
      break;
    case OrderAction::Pull:
      PullOrder(r->order);
      break;
    case OrderAction::Amend:
      AmendOrder(r->order);
      break;
    default:
      assert(false);
      break;
  }
}

void TraderApi::OnQuoteRequest(const QuoteRequestPtr &r)
{
  assert(r);
  switch (r->action)
  {
    case OrderAction::New:
      NewQuote(r->bid, r->ask);
    case OrderAction::Pull:
      PullQuote(r->bid, r->ask);
    case OrderAction::Amend:
      AmendQuote(r->bid, r->ask);
    default:
      assert(false);
  }
}
