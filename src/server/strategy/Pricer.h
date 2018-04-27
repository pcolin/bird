#ifndef STRATEGY_PRICER_H
#define STRATEGY_PRICER_H

#include "Strategy.h"
#include "PricingModel.h"
#include "VolatilityModel.h"
#include "PricingSpec.pb.h"
#include "InterestRate.pb.h"
#include "SSRate.pb.h"
#include "Volatility.pb.h"
#include "model/Option.h"


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
  struct Parameter
  {
    std::shared_ptr<double> rate = nullptr;
    std::shared_ptr<double> basis = nullptr;
    std::shared_ptr<Model::VolatilityModel::Parameter> volatility = nullptr;
  };

  void Initialize(const std::shared_ptr<Proto::PricingSpec> pricing);
  bool OnPricingSpec(const std::shared_ptr<Proto::PricingSpec> &msg);
  bool OnInterestRateReq(const std::shared_ptr<Proto::InterestRateReq> &req);
  bool OnSSRateReq(const std::shared_ptr<Proto::SSRateReq> &req);
  bool OnVolatilityCurveReq(const std::shared_ptr<Proto::VolatilityCurveReq> &req);
  TheoMatrixPtr CalculateTheo(const Option* op, const std::shared_ptr<Parameter> &param,
      base::TickType lower, base::TickType upper, double time_value);

  // std::shared_ptr<Proto::PricingSpec> pricing_;
  std::shared_ptr<Model::PricingModel> model_;
  std::shared_ptr<Model::VolatilityModel> vol_model_;

  base::PriceType spot_ = base::PRICE_UNDEFINED;
  base::TickType lower_ = INT_MAX;
  base::TickType upper_ = INT_MIN;

  std::unordered_map<const Option*, std::shared_ptr<Parameter>> option_parameters_;
};

#endif
