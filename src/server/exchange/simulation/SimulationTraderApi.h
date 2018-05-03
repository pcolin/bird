#ifndef SIMULATION_TRADER_API_H
#define SIMULATION_TRADER_API_H

#include "../manager/TraderApi.h"

class SimulationTraderApi : public TraderApi
{
  struct InstrumentConfig
  {
    std::string hedge_underlying;
    std::string product;
  };
  typedef std::unordered_map<std::string, InstrumentConfig> InstrumentConfigMap;

public:
  void Init() override;
  virtual void Login() {}
  virtual void Logout() {}

protected:
  void NewOrder(const OrderPtr &order) override;
  void NewQuote(const OrderPtr &bid, const OrderPtr &ask) override;
  void AmendOrder(const OrderPtr &order) override;
  void AmendQuote(const OrderPtr &bid, const OrderPtr &ask) override;
  void PullOrder(const OrderPtr &order) override;
  void PullQuote(const OrderPtr &bid, const OrderPtr &ask) override;

  virtual void QueryCash() override;
};

#endif
