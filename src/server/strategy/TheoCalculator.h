#ifndef STRATEGY_THEO_CALCULATOR_H
#define STRATEGY_THEO_CALCULATOR_H

#include "DeviceManager.h"
#include "PricingModel.h"
#include "VolatilityModel.h"
#include "Heartbeat.pb.h"
#include "PricingSpec.pb.h"
#include "ExchangeParameter.pb.h"
#include "InterestRate.pb.h"
#include "SSRate.pb.h"
#include "Volatility.pb.h"
#include "base/concurrency/concurrentqueue.h"
#include "model/Price.h"
#include "model/Trade.h"
#include "model/Option.h"
#include "boost/variant.hpp"

#include <thread>

class TheoCalculator
{
  typedef boost::variant<PricePtr,
                         TradePtr,
                         std::shared_ptr<Proto::Heartbeat>,
                         std::shared_ptr<Proto::VolatilityCurve>,
                         std::shared_ptr<Proto::SSRate>,
                         std::shared_ptr<Proto::PricingSpec>,
                         std::shared_ptr<Proto::ExchangeParameterReq>,
                         std::shared_ptr<Proto::InterestRateReq>> CalculatorEvent;

  struct Parameter
  {
    std::shared_ptr<double> rate = nullptr;
    std::shared_ptr<double> basis = nullptr;
    std::shared_ptr<Model::VolatilityModel::Parameter> volatility = nullptr;
  };
  typedef std::unordered_map<const Option*, std::shared_ptr<Parameter>> ParameterMap;
  typedef std::map<boost::gregorian::date, ParameterMap> MaturityParameterMap;
public:
  TheoCalculator(const std::string &name, DeviceManager *dm);

  void Start();
  void Stop();

  template<class E> bool OnEvent(const E& e) { return running_ && events_.enqueue(e); }
private:
  bool Initialize(const std::shared_ptr<Proto::PricingSpec> &spec);

  void OnPrice(const PricePtr &price);
  void OnTrade(const TradePtr &t);
  void OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &h);
  void OnPricingSpec(const std::shared_ptr<Proto::PricingSpec> &spec);
  void OnExchangeParameter(const std::shared_ptr<Proto::ExchangeParameterReq> &req);
  void OnInterestRate(const std::shared_ptr<Proto::InterestRateReq> &req);
  void OnSSRate(const std::shared_ptr<Proto::SSRate> &ssr);
  void OnVolatilityCurve(const std::shared_ptr<Proto::VolatilityCurve> &vc);

  int64_t RecalculateAll();
  void Recalculate(const boost::gregorian::date &maturity, ParameterMap &parameters,
      base::TickType lower, base::TickType upper);
  TheoMatrixPtr CalculateTheo(const Option* op, const std::shared_ptr<Parameter> &param,
      base::TickType lower, base::TickType upper, double time_value);

  void SetLowerUpper(base::TickType &lower, base::TickType &upper)
  {
    lower = std::max(1, tick_ - TheoMatrix::DEPTH);
    upper = tick_ + TheoMatrix::DEPTH;
  }

private:
  class EventVisitor : public boost::static_visitor<void>
  {
  public:
    EventVisitor(TheoCalculator *calculator) : calculator_(calculator) {}

    void operator()(const PricePtr &p) const
    {
      calculator_->OnPrice(p);
    }

    void operator()(const TradePtr &t) const
    {
      calculator_->OnTrade(t);
    }

    void operator()(const std::shared_ptr<Proto::Heartbeat> &h)
    {
      calculator_->OnHeartbeat(h);
    }

    void operator()(const std::shared_ptr<Proto::PricingSpec> &spec)
    {
      calculator_->OnPricingSpec(spec);
    }

    void operator()(const std::shared_ptr<Proto::ExchangeParameterReq> &req)
    {
      calculator_->OnExchangeParameter(req);
    }

    void operator()(const std::shared_ptr<Proto::InterestRateReq> &req)
    {
      calculator_->OnInterestRate(req);
    }

    void operator()(const std::shared_ptr<Proto::SSRate> &ssr)
    {
      calculator_->OnSSRate(ssr);
    }

    void operator()(const std::shared_ptr<Proto::VolatilityCurve> &vc)
    {
      calculator_->OnVolatilityCurve(vc);
    }

  private:
    TheoCalculator *calculator_;
  } visitor_;

  std::string name_;
  DeviceManager *dm_;

  moodycamel::ConcurrentQueue<CalculatorEvent> events_;
  std::atomic<bool> running_ = {false};
  std::unique_ptr<std::thread> thread_;

  std::shared_ptr<Model::PricingModel> model_;
  std::shared_ptr<Model::VolatilityModel> vol_model_;

  // base::PriceType spot_ = base::PRICE_UNDEFINED;
  base::TickType tick_ = 0;
  base::TickType lower_ = INT_MAX;
  base::TickType upper_ = INT_MIN;

  int64_t interval_;
  int64_t calculate_time_;

  MaturityParameterMap parameters_;
};

#endif
