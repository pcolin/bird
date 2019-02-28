#ifndef SIMULATION_TRADER_API_H
#define SIMULATION_TRADER_API_H

#include "../manager/TraderApi.h"

class SimulationTraderApi : public TraderApi {
  struct InstrumentConfig {
    std::string hedge_underlying;
    std::string product;
  };
  typedef std::unordered_map<std::string, InstrumentConfig> InstrumentConfigMap;

 public:
  void Init() override;
  virtual void Login() {}
  virtual void Logout() {}

  virtual bool IsMMOption(const Instrument *option) { return true; }
  virtual bool GetMMPrice(
      const Instrument *option,
      double theo,
      double &bid,
      double &ask) {} 
  virtual bool GetQRPrice(
      const Instrument *option,
      double theo,
      double &bid,
      double &ask) {}
  virtual bool MeetMMObligation(const OrderPtr &bid, const OrderPtr &ask, double &ratio){} 
  virtual bool MeetQRObligation(const OrderPtr &bid, const OrderPtr &ask, double &ratio){} 

 protected:
  void SubmitOrder(const OrderPtr &order) override;
  void SubmitQuote(const OrderPtr &bid, const OrderPtr &ask) override;
  void AmendOrder(const OrderPtr &rder) override;
  void AmendQuote(const OrderPtr &bid, const OrderPtr &ask) override;
  void CancelOrder(const OrderPtr &order) override;
  void CancelQuote(const OrderPtr &bid, const OrderPtr &ask) override;

  virtual void QueryCash() override;

 private:
  void MatchingProcess();

  std::unique_ptr<std::thread> td_;
};

#endif // SIMULATION_TRADER_API_H
