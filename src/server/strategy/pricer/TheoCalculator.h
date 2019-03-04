#ifndef STRATEGY_THEO_CALCULATOR_H
#define STRATEGY_THEO_CALCULATOR_H

#include <thread>
#include "strategy/base/DeviceManager.h"
#include "PricingModel.h"
#include "VolatilityModel.h"
#include "Heartbeat.pb.h"
#include "Pricer.pb.h"
#include "ProductParameter.pb.h"
#include "InterestRate.pb.h"
#include "SSRate.pb.h"
#include "Volatility.pb.h"
#include "base/concurrency/concurrentqueue.h"
#include "model/Price.h"
#include "model/Trade.h"
#include "model/Option.h"
#include "boost/variant.hpp"

class TheoCalculator {
  typedef boost::variant<PricePtr,
                         TradePtr,
                         std::shared_ptr<Proto::Heartbeat>,
                         std::shared_ptr<Proto::VolatilityCurve>,
                         std::shared_ptr<Proto::SSRate>,
                         std::shared_ptr<Proto::Pricer>,
                         std::shared_ptr<Proto::ProductParameterReq>,
                         std::shared_ptr<Proto::InterestRateReq>> CalculatorEvent;
  typedef std::vector<std::array<const Option*, 2>> OptionsVector;

  struct Parameters
  {
    double rate = NAN;
    double basis = NAN;
    std::shared_ptr<Model::VolatilityModel::Parameter> volatility = nullptr;
    // std::unordered_map<double, std::array<const Option*, 2>> options;
    std::vector<std::array<const Option*, 2>> options;
  };
  typedef std::map<boost::gregorian::date, Parameters> ParametersMap;

 public:
  TheoCalculator(const std::string &name, DeviceManager *dm);

  void Start();
  void Stop();

  template<class E> bool OnEvent(const E& e) { return running_ && events_.enqueue(e); }
 private:
  bool Initialize(const std::shared_ptr<Proto::Pricer> &spec);

  void OnPrice(const PricePtr &price);
  void OnTrade(const TradePtr &t);
  void OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &h);
  void OnPricer(const std::shared_ptr<Proto::Pricer> &spec);
  void OnProductParameter(const std::shared_ptr<Proto::ProductParameterReq> &req);
  void OnInterestRate(const std::shared_ptr<Proto::InterestRateReq> &req);
  void OnSSRate(const std::shared_ptr<Proto::SSRate> &ssr);
  void OnVolatilityCurve(const std::shared_ptr<Proto::VolatilityCurve> &vc);

  void Recalculate(ParametersMap::value_type &value);
  void CalculateAndPublish(const Option *call, const Option *put, Parameters &p, double time_value);
  void CalculateAndPublish(const Option *option, Parameters &p, double time_value);

  // void SetLowerUpper(base::TickType &lower, base::TickType &upper) {
  //   lower = std::max(1, tick_ - TheoMatrix::DEPTH);
  //   upper = tick_ + TheoMatrix::DEPTH;
  // }

 private:
  class EventVisitor : public boost::static_visitor<void> {
   public:
    EventVisitor(TheoCalculator *calculator) : calculator_(calculator) {}

    void operator()(const PricePtr &p) const { calculator_->OnPrice(p); }
    void operator()(const TradePtr &t) const { calculator_->OnTrade(t); }
    void operator()(const std::shared_ptr<Proto::Heartbeat> &h) { calculator_->OnHeartbeat(h); }
    void operator()(const std::shared_ptr<Proto::Pricer> &spec) { calculator_->OnPricer(spec); }
    void operator()(const std::shared_ptr<Proto::ProductParameterReq> &req) {
      calculator_->OnProductParameter(req);
    }
    void operator()(const std::shared_ptr<Proto::InterestRateReq> &req) {
      calculator_->OnInterestRate(req);
    }
    void operator()(const std::shared_ptr<Proto::SSRate> &ssr) { calculator_->OnSSRate(ssr); }
    void operator()(const std::shared_ptr<Proto::VolatilityCurve> &vc) {
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

  bool future_;
  std::shared_ptr<Model::PricingModel> model_;
  std::shared_ptr<Model::VolatilityModel> vol_model_;

  base::PriceType spot_ = base::PRICE_UNDEFINED;
  base::TickType lower_ = INT_MAX;
  base::TickType upper_ = INT_MIN;

  int64_t interval_;
  int64_t calculate_time_;

  ParametersMap parameters_;
};

#endif // STRATEGY_THEO_CALCULATOR_H
