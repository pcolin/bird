#ifndef STRATEGY_DIMER_H
#define STRATEGY_DIMER_H

#include "strategy/base/Strategy.h"

class Dimer : public Strategy {

 public:
  Dimer(const std::string &name, DeviceManager *dm);

  virtual void OnStart() override;
  virtual void OnStop() override;

 protected:
  virtual void OnPrice(const PricePtr &price) override;
  virtual void OnTheoMatrix(const TheoMatrixPtr &theo) override;
  virtual void OnOrder(const OrderPtr &order) override;
  virtual void OnTrade(const TradePtr &trade) override;
  virtual bool OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat) override;

 private:
};

#endif
