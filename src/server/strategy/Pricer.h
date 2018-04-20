#ifndef STRATEGY_PRICER_H
#define STRATEGY_PRICER_H

#include "Strategy.h"
#include "PricingSpec.pb.h"
#include "InterestRate.pb.h"
#include "SSRate.pb.h"
#include "Volatility.pb.h"

class Pricer : public Strategy
{
public:
  Pricer(const std::string &name, DeviceManager *dm);

  virtual void OnStart() override;
  virtual void OnStop() override;

protected:
  virtual void OnPrice(const PricePtr &price) override;
  virtual void OnTrade(const TradePtr &trade) override;

private:
  bool OnPricingSpec(const std::shared_ptr<Proto::PricingSpec> &msg);
  bool OnInterestRateReq(const std::shared_ptr<Proto::InterestRateReq> &req);
  bool OnSSRateReq(const std::shared_ptr<Proto::SSRateReq> &req);
  bool OnVolatilityCurveReq(const std::shared_ptr<Proto::VolatilityCurveReq> &req);
};

#endif
