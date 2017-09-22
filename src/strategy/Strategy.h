#ifndef STRATEGY_STRATEGY_H
#define STRATEGY_STRATEGY_H

#include "StrategyTypes.h"
#include "DeviceManager.h"

class Strategy
{
public:
  Strategy(const std::string &name, DeviceManager *dm)
    : name_(name), dm_(dm), visitor_(this)
  {}
  virtual ~Strategy() {}

  virtual void OnStart() = 0;
  virtual void OnStop() = 0;

  const std::string& Name() const { return name_; }

  void OnEvent(const Event &e, int64_t seq, bool last)
  {
    boost::apply_visitor(visitor_, e);
  }

protected:
  virtual void OnPrice(const PricePtr &price) = 0;
  virtual void OnOrder(const OrderPtr &order) = 0;
  virtual void OnTrade(const TradePtr &trade) = 0;

  const std::string name_;
  DeviceManager *dm_;

private:
  class EventVisitor : public boost::static_visitor<void>
  {
  public:
    EventVisitor(Strategy *s) : s_(s) {}

    void operator()(const PricePtr &price) const
    {
      s_->OnPrice(price);
    }
    void operator()(const OrderPtr &order) const
    {
      s_->OnOrder(order);
    }
    void operator()(const TradePtr &trade) const
    {
      s_->OnTrade(trade);
    }

  private:
    Strategy *s_;
  } visitor_;
};

#endif
