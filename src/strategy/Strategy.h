#ifndef STRATEGY_STRATEGY_H
#define STRATEGY_STRATEGY_H

#include "StrategyTypes.h"
#include "DeviceManager.h"

class Strategy
{
public:
  Strategy(DeviceManager *device) : device_(device), visitor_(this) {}

  virtual void OnStart() = 0;
  virtual void OnStop() = 0;

  void OnEvent(const Event &e, int64_t seq, bool last)
  {
    boost::apply_visitor(visitor_, e);
  }

protected:
  virtual void OnPrice(const std::shared_ptr<Price> &price) = 0;
  virtual void OnOrder(const std::shared_ptr<Order> &order) = 0;
  virtual void OnTrade(const std::shared_ptr<Trade> &trade) = 0;

  DeviceManager *device_;

private:
  class EventVisitor : public boost::static_visitor<void>
  {
  public:
    EventVisitor(Strategy *s) : s_(s) {}

    void operator()(const std::shared_ptr<Price> &price) const
    {
      s_->OnPrice(price);
    }
    void operator()(const std::shared_ptr<Order> &order) const
    {
      s_->OnOrder(order);
    }
    void operator()(const std::shared_ptr<Trade> &trade) const
    {
      s_->OnTrade(trade);
    }

  private:
    Strategy *s_;
  } visitor_;
};

#endif
