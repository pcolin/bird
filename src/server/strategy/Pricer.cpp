#include "Pricer.h"
#include "ClusterManager.h"

Pricer::Pricer(const std::string &name, DeviceManager *dm)
  : Strategy(name, dm), calc_(name, dm)
{
  dispatcher_.RegisterCallback<Proto::Heartbeat>([&](auto &m) { return calc_.OnEvent(m); });
  dispatcher_.RegisterCallback<Proto::Pricer>([&](auto &m) { return calc_.OnEvent(m); });
  dispatcher_.RegisterCallback<Proto::ExchangeParameterReq>([&](auto &m) {return calc_.OnEvent(m);});
  dispatcher_.RegisterCallback<Proto::InterestRateReq>([&](auto &m) { return calc_.OnEvent(m); });
  dispatcher_.RegisterCallback<Proto::SSRate>([&](auto &m) { return calc_.OnEvent(m); });
  dispatcher_.RegisterCallback<Proto::VolatilityCurve>([&](auto &m) { return calc_.OnEvent(m); });
}

void Pricer::OnStart()
{
  calc_.Start();
}

void Pricer::OnStop()
{
  calc_.Stop();
}

void Pricer::OnPrice(const PricePtr &price)
{
  if (price->instrument == dm_->GetUnderlying())
  {
    calc_.OnEvent(price);
  }
}

void Pricer::OnTrade(const TradePtr &trade)
{
  if (trade->instrument->Type() == Proto::InstrumentType::Option)
  {
    calc_.OnEvent(trade);
  }
}

bool Pricer::OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat)
{
  Strategy::OnHeartbeat(heartbeat);
  return calc_.OnEvent(heartbeat);
}
