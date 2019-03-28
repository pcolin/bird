#include "TheoCalculator.h"
#include "strategy/base/ClusterManager.h"
#include "BlackScholesMertonModel.h"
#include "CoxRossRubinsteinModel.h"
#include "base/logger/Logging.h"
#include "base/common/Likely.h"
#include "base/common/Float.h"
#include "model/InstrumentManager.h"
#include "model/ParameterManager.h"
#include "model/PositionManager.h"
#include "boost/format.hpp"
// #include "tbb/parallel_for.h"
#include "tbb/parallel_for_each.h"
// #include "tbb/blocked_range.h"

TheoCalculator::TheoCalculator(const std::string &name, DeviceManager *dm)
    : visitor_(this), name_(name), dm_(dm) {}

void TheoCalculator::Start() {
  auto spec = ClusterManager::GetInstance()->FindPricer(name_);
  if (spec) {
    if (Initialize(spec)) {
      running_ = true;
      thread_ = std::make_unique<std::thread>(
          [&]() {
            LOG_INF << "start calculator thread...";
            while (running_) {
              CalculatorEvent e;
              if (events_.try_dequeue(e)) {
                boost::apply_visitor(visitor_, e);
              }
            }
          });
    }
  } else {
    LOG_ERR << boost::format("pricing %1% is not existed") % name_;
  }
}

void TheoCalculator::Stop() {
  running_ = false;
  if (thread_) {
    thread_->join();
    thread_.reset();
  }
}

bool TheoCalculator::Initialize(const std::shared_ptr<Proto::Pricer> &spec) {
  Proto::PricingModel model = spec->model();
  LOG_INF << boost::format("start running %1%: model(%2%)") % spec->name() %
             Proto::PricingModel_Name(model);
  if (model == Proto::PricingModel::BSM) {
    model_ = std::make_shared<Model::BlackScholesMertonModel>();
  } else if (model == Proto::PricingModel::CRR) {
    model_ = std::make_shared<Model::CoxRossRubinsteinModel>();
  } else {
    LOG_ERR << "unknown pricing model";
    return false;
  }
  vol_model_ = std::make_shared<Model::VolatilityModel>();
  interval_ = spec->interval() * base::MILLION;
  // spot_ = base::PRICE_UNDEFINED;
  parameters_.clear();
  auto *underlying = dm_->GetUnderlying();
  future_ = underlying->Type() == Proto::InstrumentType::Future;
  auto options = InstrumentManager::GetInstance()->FindOptions(underlying);
  std::sort(options.begin(), options.end(),
            [](const Option *op1, const Option *op2) {
              if (op1->Underlying()->Id() < op2->Underlying()->Id()) {
                return true;
              } else if (op1->Underlying()->Id() == op2->Underlying()->Id()) {
                if (op1->Strike() < op2->Strike()) {
                  return true;
                } else if (op1->Strike() == op2->Strike()) {
                  return op1->CallPut() < op2->CallPut();
                }
              }
              return false;
            });
  assert (options.size() % 2 == 0);
  for (int i = 0; i < options.size(); i += 2) {
    const Option *call = options[i];
    const Option *put = options[i + 1];
    // auto *option = base::down_cast<const Option*>(op);
    assert (call && put);
    auto &maturity = call->Maturity();
    auto it = parameters_.find(maturity);
    if (it == parameters_.end()) {
      it = parameters_.emplace(maturity, Parameters()).first;
      auto *pm = ParameterManager::GetInstance();
      double tmp = 0;
      if (pm->GetInterestRate(maturity, tmp)) {
        it->second.rate = tmp;
      }

      if (pm->GetSSRate(underlying, maturity, tmp)) {
        it->second.basis = tmp;
      }

      auto vc = pm->GetVolatilityCurve(underlying, maturity);
      if (vc) {
        it->second.volatility = std::make_shared<Model::VolatilityModel::Parameter>();
        it->second.volatility->spot = vc->spot();
        it->second.volatility->atm_vol = vc->atm_vol();
        it->second.volatility->call_convex = vc->call_convex();
        it->second.volatility->put_convex = vc->put_convex();
        it->second.volatility->call_slope = vc->call_slope();
        it->second.volatility->put_slope = vc->put_slope();
        it->second.volatility->call_cutoff = vc->call_cutoff();
        it->second.volatility->put_cutoff = vc->put_cutoff();
        it->second.volatility->vcr = vc->vcr();
        it->second.volatility->scr = vc->scr();
        it->second.volatility->ccr = vc->ccr();
        it->second.volatility->spcr = vc->spcr();
        it->second.volatility->sccr = vc->sccr();
      } else {/// if (param->volatility)
        // it->second.volatility.reset();
        LOG_WAN << boost::format("volatility curve of %1% for %2% is null") %
                   to_iso_string(maturity) % underlying->Id();
      }
    }
    it->second.options.insert(it->second.options.cend(), {call, put});
    LOG_DBG << boost::format("add options : %1% and %2%") % call->Id() % put->Id();
  }
  return true;
}

void TheoCalculator::OnPrice(const PricePtr &price) {
  price->header.SetInterval(0);
  LOG_DBG << price;
  static double half_distance = price->instrument->Tick() * TheoMatrix::DEPTH / 4;
  if (spot_ == base::PRICE_UNDEFINED || std::fabs(price->adjusted_price - spot_) >= half_distance) {
    spot_ = price->adjusted_price;
    auto tick = price->instrument->ConvertToHalfTick(spot_);
    lower_ = std::max(1, tick - TheoMatrix::DEPTH);
    upper_ = tick + TheoMatrix::DEPTH;

    auto begin_time = base::Now();
    tbb::parallel_for_each(parameters_.begin(), parameters_.end(),
                           [&](ParametersMap::value_type &v) {
                             if (!isnan(v.second.rate) && !isnan(v.second.basis) &&
                                 v.second.volatility) {
                               Recalculate(v);
                             }
                           });
    calculate_time_ = base::Now();
    LOG_INF << boost::format("recalc theo with spot(%1%), matrix[%2%, %3%] finished : %4%us") %
               price->adjusted_price % lower_ % upper_ % (calculate_time_ - begin_time);
  }
}

void TheoCalculator::OnTrade(const TradePtr &trade) {
  trade->header.SetInterval(0);
  const Option* op = base::down_cast<const Option*>(trade->instrument);
  assert(op);
  auto it = parameters_.find(op->Maturity());
  if (it != parameters_.end() && !isnan(it->second.rate) && !isnan(it->second.basis) &&
      it->second.volatility) {
    int beg = 0, end = it->second.options.size();
    while (beg < end) {
      int mid = (beg + end) / 2;
      auto *call = it->second.options[mid][0];
      assert(call);
      if (op->Strike() == call->Strike()) {
        auto product = ParameterManager::GetInstance()->GetProduct(
            op->Underlying()->Product());
        if (product) {
          double t = product->GetTimeValue(op->Maturity());
          CalculateAndPublish(it->second.options[mid][op->CallPut()], it->second, t);
        }
        return;
      } else if (op->Strike() > call->Strike()) {
        beg = mid + 1;
      } else {
        end = mid;
      }
    }
  }
}

void TheoCalculator::OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &h) {
  if (base::Now() - calculate_time_ > interval_ && spot_ != base::PRICE_UNDEFINED) {
    auto begin_time = base::Now();
    tbb::parallel_for_each(parameters_.begin(), parameters_.end(),
                           [&](ParametersMap::value_type &v) {
                             if (!isnan(v.second.rate) && !isnan(v.second.basis) &&
                                 v.second.volatility) {
                               Recalculate(v);
                             }
                           });
    calculate_time_ = base::Now();
    LOG_INF << boost::format("timely recalc with matrix[%1%, %2%] finished: %3%us") %
               lower_ % upper_ % (calculate_time_ - begin_time);
  }
}

void TheoCalculator::OnPricer(const std::shared_ptr<Proto::Pricer> &spec) {
  assert(spec->name() == name_);
  if (Initialize(spec) && spot_ != base::PRICE_UNDEFINED) {
    auto begin_time = base::Now();
    tbb::parallel_for_each(parameters_.begin(), parameters_.end(),
                           [&](ParametersMap::value_type &v) {
                             if (!isnan(v.second.rate) && !isnan(v.second.basis) &&
                                 v.second.volatility) {
                               Recalculate(v);
                             }
                           });
    calculate_time_ = base::Now();
    LOG_INF << boost::format("update Pricer to recalc with matrix[%1%, %2%] finished: %3%us") %
               lower_ % upper_ % (calculate_time_ - begin_time);
  }
}

void TheoCalculator::OnProductParameter(const std::shared_ptr<Proto::ProductParameterReq> &req) {
  if (spot_ != base::PRICE_UNDEFINED) {
    auto begin_time = base::Now();
    tbb::parallel_for_each(parameters_.begin(), parameters_.end(),
                           [&](ParametersMap::value_type &v) {
                             if (!isnan(v.second.rate) && !isnan(v.second.basis) &&
                                 v.second.volatility) {
                               Recalculate(v);
                             }
                           });
    calculate_time_ = base::Now();
    LOG_INF << boost::format("update ProductParameter to recalc with matrix[%1%, %2%] "
                             "finished: %3%us") % lower_ % upper_ % (calculate_time_ - begin_time);
  }
}

void TheoCalculator::OnInterestRate(const std::shared_ptr<Proto::InterestRateReq> &req) {
  for(auto &it : parameters_) {
    ParameterManager::GetInstance()->GetInterestRate(it.first, it.second.rate);
  }
  if (spot_ != base::PRICE_UNDEFINED) {
    auto begin_time = base::Now();
    tbb::parallel_for_each(parameters_.begin(), parameters_.end(),
                           [&](ParametersMap::value_type &v) {
                             if (!isnan(v.second.basis) && v.second.volatility) {
                               Recalculate(v);
                             }
                           });
    calculate_time_ = base::Now();
    LOG_INF << boost::format("update InterestRate to recalc with matrix[%1%, %2%] finished: %3%us") %
               lower_ % upper_ % (calculate_time_ - begin_time);
  }
}

void TheoCalculator::OnSSRate(const std::shared_ptr<Proto::SSRate> &ssr) {
  auto maturity = boost::gregorian::from_undelimited_string(ssr->maturity());
  auto it = parameters_.find(maturity);
  if (it != parameters_.end()) {
    it->second.basis = ssr->rate();
  } else {
    LOG_ERR << "can't find maturity " << to_iso_string(maturity);
    return;
  }
  if (spot_ != base::PRICE_UNDEFINED) {
    auto begin_time = base::Now();
    tbb::parallel_for_each(parameters_.begin(), parameters_.end(),
                           [&](ParametersMap::value_type &v) {
                             if (!isnan(v.second.rate) && v.second.volatility) {
                               Recalculate(v);
                             }
                           });
    calculate_time_ = base::Now();
    LOG_INF << boost::format("update SSRate to recalc with matrix[%1%, %2%] finished: %3%us") %
               lower_ % upper_ % (calculate_time_ - begin_time);
  }
}

void TheoCalculator::OnVolatilityCurve(const std::shared_ptr<Proto::VolatilityCurve> &vc) {
  auto maturity = boost::gregorian::from_undelimited_string(vc->maturity());
  auto it = parameters_.find(maturity);
  if (it != parameters_.end()) {
    if (!it->second.volatility) {
      it->second.volatility = std::make_shared<Model::VolatilityModel::Parameter>();
    }
    it->second.volatility->spot = vc->spot();
    it->second.volatility->atm_vol = vc->atm_vol();
    it->second.volatility->call_convex = vc->call_convex();
    it->second.volatility->put_convex = vc->put_convex();
    it->second.volatility->call_slope = vc->call_slope();
    it->second.volatility->put_slope = vc->put_slope();
    it->second.volatility->call_cutoff = vc->call_cutoff();
    it->second.volatility->put_cutoff = vc->put_cutoff();
    it->second.volatility->vcr = vc->vcr();
    it->second.volatility->scr = vc->scr();
    it->second.volatility->ccr = vc->ccr();
    it->second.volatility->spcr = vc->spcr();
    it->second.volatility->sccr = vc->sccr();
  } else {
    LOG_ERR << "can't find maturity " << to_iso_string(maturity);
    return;
  }
  if (spot_ != base::PRICE_UNDEFINED) {
    auto begin_time = base::Now();
    tbb::parallel_for_each(parameters_.begin(), parameters_.end(),
                           [&](ParametersMap::value_type &v) {
                             if (!isnan(v.second.rate) && !isnan(v.second.basis)) {
                               Recalculate(v);
                             }
                           });
    calculate_time_ = base::Now();
    LOG_INF << boost::format("update volatility to recalc with matrix[%1%, %2%] finished: %3%us") %
               lower_ % upper_ % (calculate_time_ - begin_time);
  }
}

void TheoCalculator::Recalculate(ParametersMap::value_type &value) {
  static int counts[5] = { 1, 2, 4, 8, 16 };
  static int count = 0;
  auto *underlying = dm_->GetUnderlying();
  auto product = ParameterManager::GetInstance()->GetProduct(underlying->Product());
  if (product) {
    double t = product->GetTimeValue(value.first);
    // tbb::parallel_for_each(value.second.options.begin(), value.second.options.end(),
    //     [&](std::array<const Option*, 2> &options)
    //     {
    //       CalculateAndPublish(options[0], options[1], value.second, t, lower, upper);
    //     });
    auto &options = value.second.options;
    tbb::parallel_for(tbb::blocked_range<OptionsVector::iterator>(options.begin(), options.end(),
                                                                  counts[count % 5]),
                      [&](const tbb::blocked_range<OptionsVector::iterator> &r) {
                        for (auto it = r.begin(); it != r.end(); ++it)
                        {
                          CalculateAndPublish((*it)[0], (*it)[1], value.second, t);
                        }
                      });
    LOG_DBG << "parallel with " << counts[count % 5];
    ++count;
  } else {
    LOG_ERR << "failed to get parameter of product " << underlying->Product();
  }
}

void TheoCalculator::CalculateAndPublish(const Option *call,
                                         const Option *put,
                                         Parameters &p,
                                         double time_value) {
  auto call_matrix = std::make_shared<TheoMatrix>();
  call_matrix->option = call;
  call_matrix->lower = lower_;
  call_matrix->upper = upper_;
  double call_adjust = 0;
  if (ParameterManager::GetInstance()->GetDestriker(call, call_adjust)) {
    call_adjust *= PositionManager::GetInstance()->GetNetPosition(call);
  }
  auto put_matrix = std::make_shared<TheoMatrix>();
  put_matrix->option = put;
  put_matrix->lower = lower_;
  put_matrix->upper = upper_;
  double put_adjust = 0;
  if (ParameterManager::GetInstance()->GetDestriker(put, put_adjust)) {
    put_adjust *= PositionManager::GetInstance()->GetNetPosition(put);
  }
  auto lower = call->HedgeUnderlying()->ConvertHalfToPrice(lower_);
  auto half_tick = call->HedgeUnderlying()->Tick() / 2;
  for (auto i = 0; i <= upper_ - lower_; ++i) {
    double s = lower + i * half_tick;
    double k = call->Strike();
    double v = vol_model_->Calculate(*p.volatility, future_, s, k, p.rate, p.basis, time_value);

    Model::TheoData call_theo, put_theo;
    if (future_) {
      model_->Calculate(true, s + p.basis, k, v, p.rate, p.rate, time_value, call_theo);
      model_->Calculate(false, s + p.basis, k, v, p.rate, p.rate, time_value, put_theo);
    } else {
      model_->Calculate(true, s, k, v, p.rate, p.basis, time_value, call_theo);
      model_->Calculate(false, s, k, v, p.rate, p.basis, time_value, put_theo);
    }

    call_matrix->theos[i].spot = s;
    call_matrix->theos[i].volatility = v;
    call_matrix->theos[i].ss_rate = p.basis;
    call_matrix->theos[i].theo = std::max(0.0, call_theo.theo - call_adjust);
    call_matrix->theos[i].delta = call_theo.delta;
    call_matrix->theos[i].gamma = call_theo.gamma;
    call_matrix->theos[i].theta = call_theo.theta;
    put_matrix->theos[i].spot = s;
    put_matrix->theos[i].volatility = v;
    put_matrix->theos[i].ss_rate = p.basis;
    put_matrix->theos[i].theo = std::max(0.0, put_theo.theo - put_adjust);
    put_matrix->theos[i].delta = put_theo.delta;
    put_matrix->theos[i].gamma = put_theo.gamma;
    put_matrix->theos[i].theta = put_theo.theta;
    LOG_DBG << boost::format("calculate %1%: s(%2%), v(%3%), r(%4%), q(%5%), t(%6%), "
        "theo(%7%), delta(%8%), gamma(%9%), theta(%10%)") % call->Id() % s % v % p.rate %
        p.basis % time_value % call_theo.theo % call_theo.delta % call_theo.gamma %
        call_theo.theta;
    // LOG_DBG << boost::format("calculate %1%: s(%2%), v(%3%), r(%4%), q(%5%), t(%6%), "
    //     "theo(%7%), delta(%8%), gamma(%9%), theta(%10%)") % put->Id() % s % v % p.rate %
    //     p.basis % time_value % put_theo.theo % put_theo.delta % put_theo.gamma %
    //     put_theo.theta;
  }
  dm_->Publish(call_matrix);
  dm_->Publish(put_matrix);
}

void TheoCalculator::CalculateAndPublish(const Option *option, Parameters &p, double time_value) {
  auto matrix = std::make_shared<TheoMatrix>();
  matrix->option = option;
  matrix->lower = lower_;
  matrix->upper = upper_;
  double adjust = 0;
  if (ParameterManager::GetInstance()->GetDestriker(option, adjust)) {
    adjust *= PositionManager::GetInstance()->GetNetPosition(option);
  }
  auto lower = option->HedgeUnderlying()->ConvertHalfToPrice(lower_);
  for (auto i = 0; i <= upper_ - lower_; ++i) {
    double s = lower + i * option->HedgeUnderlying()->Tick() / 2;
    double k = option->Strike();
    double v = vol_model_->Calculate(*p.volatility, future_, s, k, p.rate, p.basis, time_value);

    Model::TheoData theo;
    bool call = option->CallPut() == Proto::OptionType::Call;
    if (future_) {
      model_->Calculate(call, s + p.basis, k, v, p.rate, p.rate, time_value, theo);
    } else {
      model_->Calculate(call, s, k, v, p.rate, p.basis, time_value, theo);
    }

    matrix->theos[i].spot = s;
    matrix->theos[i].volatility = v;
    matrix->theos[i].ss_rate = p.basis;
    matrix->theos[i].theo = std::max(0.0, theo.theo - adjust);
    matrix->theos[i].delta = theo.delta;
    matrix->theos[i].gamma = theo.gamma;
    matrix->theos[i].theta = theo.theta;
    // LOG_TRA << boost::format("Calculate %1%: s(%2%), v(%3%), r(%4%), q(%5%), t(%6%), "
    //     "theo(%7%), delta(%8%), gamma(%9%), theta(%10%)") % op->Id() % s % v % r % q %
    //   t % theo.theo % theo.delta % theo.gamma % theo.theta;
  }
  dm_->Publish(matrix);
}
