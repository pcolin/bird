#include "Pricer.h"
#include "base/logger/Logging.h"
#include "exchange/manager/ExchangeManager.h"

#include <boost/format.hpp>

Pricer::Pricer(const std::string &name, DeviceManager *dm)
  : Strategy(name, dm)
{
  dispatcher_.RegisterCallback<Proto::PricingSpec>(
      std::bind(&Pricer::OnPricingSpec, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::InterestRateReq>(
      std::bind(&Pricer::OnInterestRateReq, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::SSRateReq>(
      std::bind(&Pricer::OnSSRateReq, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::VolatilityCurveReq>(
      std::bind(&Pricer::OnVolatilityCurveReq, this, std::placeholders::_1));
}

void Pricer::OnStart()
{
  LOG_INF << "OnStart";
}

void Pricer::OnStop()
{
  LOG_INF << "OnStop";
}

void Pricer::OnPrice(const PricePtr &price)
{
  if (price->instrument == dm_->GetUnderlying())
  {
    LOG_INF << "OnPrice : " << price->Dump();
  }
}

void Pricer::OnTrade(const TradePtr &trade)
{
  LOG_INF << "OnTrade: " << trade->Dump();
}

bool Pricer::OnPricingSpec(const std::shared_ptr<Proto::PricingSpec> &msg)
{
  return true;
}

bool OnInterestRateReq(const std::shared_ptr<Proto::InterestRateReq> &req)
{
  return true;
}

bool OnSSRateReq(const std::shared_ptr<Proto::SSRateReq> &req)
{
  return true;
}

bool OnVolatilityCurveReq(const std::shared_ptr<Proto::VolatilityCurveReq> &req)
{
  return true;
}
