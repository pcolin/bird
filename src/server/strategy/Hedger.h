#ifndef STRATEGY_HEDGER_H
#define STRATEGY_HEDGER_H

#include "Strategy.h"

class Hedger : public Strategy {

 public:
  Hedger(const std::string &name, DeviceManager *dm);

  virtual void OnStart() override;
  virtual void OnStop() override;

  double OpenDelta() { return open_delta_; }

 protected:
  virtual void OnPrice(const PricePtr &price) override;
  virtual void OnTheoMatrix(const TheoMatrixPtr &theo) override;
  virtual void OnOrder(const OrderPtr &order) override;
  virtual void OnTrade(const TradePtr &trade) override;
  virtual bool OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat) override;

 private:
  std::atomic<double> open_delta_;
};

#endif
