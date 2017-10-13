#ifndef STRATEGY_STRATEGY_H
#define STRATEGY_STRATEGY_H

#include "DeviceManager.h"
#include "Heartbeat.pb.h"
#include "model/ProtoMessageDispatcher.h"

class Strategy
{
public:
  Strategy(const std::string &name, DeviceManager *dm);
  virtual ~Strategy() {}

  virtual void OnStart() = 0;
  virtual void OnStop() = 0;

  const std::string& Name() const { return name_; }

  void OnEvent(const Event &e, int64_t seq, bool last)
  {
    boost::apply_visitor(visitor_, e);
    // if (last)
    //   OnLastEvent();
  }

protected:
  virtual void OnPrice(const PricePtr &price) {}
  virtual void OnOrder(const OrderPtr &order) {}
  virtual void OnTrade(const TradePtr &trade) {}
  void OnProtoMessage(const ProtoMessagePtr &message) { dispatcher_.OnProtoMessage(message); }
  virtual void OnLastEvent() {}

  const std::string name_;
  DeviceManager *dm_;
  ProtoMessageDispatcher<bool> dispatcher_;

private:
  bool OnHeartbeat(const std::shared_ptr<proto::Heartbeat> &heartbeat);

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
    void operator()(const ProtoMessagePtr &message) const
    {
      s_->OnProtoMessage(message);
    }

  private:
    Strategy *s_;
  } visitor_;
};

#endif
