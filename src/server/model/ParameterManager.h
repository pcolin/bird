#ifndef MODEL_PARAMETER_MANAGER_H
#define MODEL_PARAMETER_MANAGER_H

#include "Instrument.h"
#include "InterestRate.pb.h"
#include "SSRate.pb.h"
#include "Volatility.pb.h"
#include "Destriker.pb.h"
#include "Elastic.pb.h"

#include <mutex>

class ParameterManager
{
public:
  static ParameterManager* GetInstance();
  ~ParameterManager() {}

  void Init();
  bool GetInterestRate(const boost::gregorian::date &date, double &rate);
  bool GetSSRate(const Instrument *underlying, const boost::gregorian::date &date, double rate);
  bool GetVolatility(const Instrument *instrument, double &volatility);
  bool GetDestriker(const Instrument *instrument, double &destriker);
  bool GetElastic(const Instrument *instrument, double &elastic);

  void OnInterestRateReq(const std::shared_ptr<Proto::InterestRateReq> &req);
  void OnSSRateReq(const std::shared_ptr<Proto::SSRateReq> &req);
  void OnVolatilityReq(const std::shared_ptr<Proto::VolatilityReq> &req);
  void OnDestrikerReq(const std::shared_ptr<Proto::DestrikerReq> &req);
  void OnElasticReq(const std::shared_ptr<Proto::ElasticReq> &req);

private:
  std::map<int32_t, double> interest_rates_;
  std::mutex interest_rates_mtx_;
  typedef std::map<boost::gregorian::date, double> DateRateMap;
  std::unordered_map<const Instrument*, std::shared_ptr<DateRateMap>> ssrates_;
  std::mutex ssrates_mtx_;
  std::unordered_map<const Instrument*, double> volatilities_;
  std::mutex volatilities_mtx_;
  std::unordered_map<const Instrument*, double> destrikers_;
  std::mutex destrikers_mtx_;
  std::unordered_map<const Instrument*, double> elastics_;
  std::mutex elastics_mtx_;
};

#endif
