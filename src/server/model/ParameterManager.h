#ifndef MODEL_PARAMETER_MANAGER_H
#define MODEL_PARAMETER_MANAGER_H

#include <mutex>
#include "Exchange.h"
#include "InterestRate.pb.h"
#include "SSRate.pb.h"
#include "Volatility.pb.h"
#include "Destriker.pb.h"
#include "Elastic.pb.h"

class ParameterManager {
 public:
  typedef std::shared_ptr<Proto::Reply> ProtoReplyPtr;
  static ParameterManager* GetInstance();
  ~ParameterManager() {}

  void InitGlobal();
  void Init();
  std::shared_ptr<Exchange> GetExchange() { return exchange_; }
  bool GetInterestRate(const boost::gregorian::date &date, double &rate);
  bool GetSSRate(const Instrument *underlying, const boost::gregorian::date &date, double &rate);
  // bool GetVolatility(const Instrument *instrument, double &volatility);
  std::shared_ptr<Proto::VolatilityCurve> GetVolatilityCurve(const Instrument *underlying,
      const boost::gregorian::date &date);
  bool GetDestriker(const Instrument *instrument, double &destriker);
  bool GetElastic(const Instrument *instrument, double &elastic);

  ProtoReplyPtr OnExchangeParameterReq(const std::shared_ptr<Proto::ExchangeParameterReq> &req);
  ProtoReplyPtr OnInterestRateReq(const std::shared_ptr<Proto::InterestRateReq> &req);
  ProtoReplyPtr OnSSRateReq(const std::shared_ptr<Proto::SSRateReq> &req);
  // ProtoReplyPtr OnVolatilityReq(const std::shared_ptr<Proto::VolatilityReq> &req);
  ProtoReplyPtr OnVolatilityCurveReq(const std::shared_ptr<Proto::VolatilityCurveReq> &req);
  ProtoReplyPtr OnDestrikerReq(const std::shared_ptr<Proto::DestrikerReq> &req);
  // void OnElasticReq(const std::shared_ptr<Proto::ElasticReq> &req);

private:
  std::shared_ptr<Exchange> exchange_;

  std::map<int32_t, double> interest_rates_;
  std::mutex interest_rates_mtx_;

  typedef std::map<boost::gregorian::date, double> DateRateMap;
  std::unordered_map<const Instrument*, std::shared_ptr<DateRateMap>> ssrates_;
  std::mutex ssrates_mtx_;

  // std::unordered_map<const Instrument*, double> volatilities_;
  // std::mutex volatilities_mtx_;

  typedef std::map<boost::gregorian::date,std::shared_ptr<Proto::VolatilityCurve>> DateVolatilityMap;
  std::unordered_map<const Instrument*, std::shared_ptr<DateVolatilityMap>> volatility_curves_;
  std::mutex volatility_curves_mtx_;

  std::unordered_map<const Instrument*, double> destrikers_;
  std::mutex destrikers_mtx_;

  // std::unordered_map<const Instrument*, double> elastics_;
  // std::mutex elastics_mtx_;
};

#endif // MODEL_PARAMETER_MANAGER_H
