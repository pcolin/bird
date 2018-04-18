#include "Pricer.h"
#include "base/logger/Logging.h"
#include "exchange/manager/ExchangeManager.h"

#include <boost/format.hpp>

Pricer::Pricer(const std::string &name, DeviceManager *dm)
  : Strategy(name, dm)
{
  dispatcher_.RegisterCallback<Proto::PricingSpec>(
      std::bind(&Pricer::OnPricingSpec, this, std::placeholders::_1));
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
  LOG_INF << "OnPrice : " << price->Dump();
}

void Pricer::OnTrade(const TradePtr &trade)
{
  LOG_INF << "OnTrade: " << trade->Dump();
}

bool Pricer::OnPricingSpec(const std::shared_ptr<Proto::PricingSpec> &msg)
{
  return true;
}
