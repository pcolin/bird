#include "TheoCalculator.h"
#include "ClusterManager.h"
#include "BlackScholesMertonModel.h"
#include "CoxRossRubinsteinModel.h"
#include "base/logger/Logging.h"
#include "base/common/Likely.h"
#include "base/common/Float.h"
#include "model/ProductManager.h"
#include "model/ParameterManager.h"
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
            LOG_INF << "Start calculator thread...";
            while (running_) {
              CalculatorEvent e;
              if (events_.try_dequeue(e)) {
                boost::apply_visitor(visitor_, e);
              }
            }
          });
    }
  } else {
    LOG_ERR << boost::format("Pricing %1% is not existed") % name_;
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
  LOG_INF << boost::format("Start running %1%: model(%2%)") % spec->name() %
             Proto::PricingModel_Name(model);
  if (model == Proto::PricingModel::BSM) {
    model_ = std::make_shared<Model::BlackScholesMertonModel>();
  } else if (model == Proto::PricingModel::CRR) {
    model_ = std::make_shared<Model::CoxRossRubinsteinModel>();
  } else {
    LOG_ERR << "Unknown pricing model";
    return false;
  }
  vol_model_ = std::make_shared<Model::VolatilityModel>();
  interval_ = spec->interval() * base::MILLION;
  // spot_ = base::PRICE_UNDEFINED;
  parameters_.clear();
  auto *underlying = dm_->GetUnderlying();
  future_ = underlying->Type() == Proto::InstrumentType::Future;
  auto options = ProductManager::GetInstance()->FindOptions(underlying);
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
        LOG_WAN << boost::format("Volatility curve of %1% for %2% is null") %
                   to_iso_string(maturity) % underlying->Id();
      }
    }
    it->second.options.insert(it->second.options.cend(), {call, put});
    LOG_DBG << boost::format("Add options : %1% and %2%") % call->Id() % put->Id();
  }
  return true;
}

void TheoCalculator::OnPrice(const PricePtr &price) {
  price->header.SetInterval(0);
  LOG_DBG << price;
  tick_ = price->instrument->ConvertToHalfTick(price->adjusted_price);
  if (tick_ < lower_ || tick_ > upper_) {
    auto begin_time = base::Now();
    base::TickType lower = 0, upper = 0;
    SetLowerUpper(lower, upper);
    tbb::parallel_for_each(parameters_.begin(), parameters_.end(),
                           [&](ParametersMap::value_type &v) {
                             if (!isnan(v.second.rate) && !isnan(v.second.basis) &&
                                 v.second.volatility) {
                               Recalculate(v, lower, upper);
                             }
                           });

    calculate_time_ = base::Now();
    LOG_INF << boost::format("Recalc theo with spot(%1%), matrix[%2%, %3%] finished : %4%us") %
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
        auto exch = ParameterManager::GetInstance()->GetExchange();
        if (exch) {
          double t = exch->GetTimeValue(op->Maturity());
          base::TickType lower = 0, upper = 0;
          SetLowerUpper(lower, upper);
          CalculateAndPublish(it->second.options[mid][op->CallPut()], it->second, t, lower, upper);
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
  if (base::Now() - calculate_time_ > interval_ && tick_) {
    base::TickType lower = 0, upper = 0;
    SetLowerUpper(lower, upper);

    auto begin_time = base::Now();
    tbb::parallel_for_each(parameters_.begin(), parameters_.end(),
                           [&](ParametersMap::value_type &v) {
                             if (!isnan(v.second.rate) && !isnan(v.second.basis) &&
                                 v.second.volatility) {
                               Recalculate(v, lower, upper);
                             }
                           });
    calculate_time_ = base::Now();

    LOG_INF << boost::format("Timely recalc with matrix[%1%, %2%] finished: %3%us") %
               lower_ % upper_ % (calculate_time_ - begin_time);
  }
}

void TheoCalculator::OnPricer(const std::shared_ptr<Proto::Pricer> &spec) {
  assert(spec->name() == name_);
  if (Initialize(spec) && tick_) {
    base::TickType lower = 0, upper = 0;
    SetLowerUpper(lower, upper);

    auto begin_time = base::Now();
    tbb::parallel_for_each(parameters_.begin(), parameters_.end(),
                           [&](ParametersMap::value_type &v) {
                             if (!isnan(v.second.rate) && !isnan(v.second.basis) &&
                                 v.second.volatility) {
                               Recalculate(v, lower, upper);
                             }
                           });
    calculate_time_ = base::Now();

    LOG_INF << boost::format("Update Pricer to recalc with matrix[%1%, %2%] finished: %3%us") %
               lower_ % upper_ % (calculate_time_ - begin_time);
  }
}

void TheoCalculator::OnExchangeParameter(const std::shared_ptr<Proto::ExchangeParameterReq> &req) {
  if (tick_) {
    base::TickType lower = 0, upper = 0;
    SetLowerUpper(lower, upper);

    auto begin_time = base::Now();
    tbb::parallel_for_each(parameters_.begin(), parameters_.end(),
                           [&](ParametersMap::value_type &v) {
                             if (!isnan(v.second.rate) && !isnan(v.second.basis) &&
                                 v.second.volatility) {
                               Recalculate(v, lower, upper);
                             }
                           });
    calculate_time_ = base::Now();

    LOG_INF << boost::format("Update ExchangeParameter to recalc with matrix[%1%, %2%] "
                             "finished: %3%us") % lower_ % upper_ % (calculate_time_ - begin_time);
  }
}

void TheoCalculator::OnInterestRate(const std::shared_ptr<Proto::InterestRateReq> &req) {
  for(auto &it : parameters_) {
    ParameterManager::GetInstance()->GetInterestRate(it.first, it.second.rate);
  }
  if (tick_) {
    base::TickType lower = 0, upper = 0;
    SetLowerUpper(lower, upper);

    auto begin_time = base::Now();
    tbb::parallel_for_each(parameters_.begin(), parameters_.end(),
                           [&](ParametersMap::value_type &v) {
                             if (!isnan(v.second.basis) && v.second.volatility) {
                               Recalculate(v, lower, upper);
                             }
                           });
    calculate_time_ = base::Now();
    LOG_INF << boost::format("Update InterestRate to recalc with matrix[%1%, %2%] finished: %3%us") %
               lower_ % upper_ % (calculate_time_ - begin_time);
  }
}

void TheoCalculator::OnSSRate(const std::shared_ptr<Proto::SSRate> &ssr) {
  auto maturity = boost::gregorian::from_undelimited_string(ssr->maturity());
  auto it = parameters_.find(maturity);
  if (it != parameters_.end()) {
    it->second.basis = ssr->rate();
  } else {
    LOG_ERR << "Can't find maturity " << to_iso_string(maturity);
    return;
  }
  if (tick_) {
    base::TickType lower = 0, upper = 0;
    SetLowerUpper(lower, upper);

    auto begin_time = base::Now();
    tbb::parallel_for_each(parameters_.begin(), parameters_.end(),
                           [&](ParametersMap::value_type &v) {
                             if (!isnan(v.second.rate) && v.second.volatility) {
                               Recalculate(v, lower, upper);
                             }
                           });
    calculate_time_ = base::Now();
    LOG_INF << boost::format("Update SSRate to recalc with matrix[%1%, %2%] finished: %3%us") %
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
    LOG_ERR << "Can't find maturity " << to_iso_string(maturity);
    return;
  }
  if (tick_) {
    base::TickType lower = 0, upper = 0;
    SetLowerUpper(lower, upper);

    auto begin_time = base::Now();
    tbb::parallel_for_each(parameters_.begin(), parameters_.end(),
                           [&](ParametersMap::value_type &v) {
                             if (!isnan(v.second.rate) && !isnan(v.second.basis)) {
                               Recalculate(v, lower, upper);
                             }
                           });
    calculate_time_ = base::Now();
    LOG_INF << boost::format("Update volatility to recalc with matrix[%1%, %2%] finished: %3%us") %
               lower_ % upper_ % (calculate_time_ - begin_time);
  }
}

void TheoCalculator::Recalculate(ParametersMap::value_type &value,
                                 base::TickType lower,
                                 base::TickType upper) {
  static int counts[5] = { 1, 2, 4, 8, 16 };
  static int count = 0;
  double t = ParameterManager::GetInstance()->GetExchange()->GetTimeValue(value.first);
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
                        CalculateAndPublish((*it)[0], (*it)[1], value.second, t, lower, upper);
                      }
                    });
  lower_ = lower;
  upper_ = upper;
  LOG_DBG << "parallel with " << counts[count % 5];
  ++count;
}

void TheoCalculator::CalculateAndPublish(const Option *call,
                                         const Option *put,
                                         Parameters &p,
                                         double time_value,
                                         base::TickType lower,
                                         base::TickType upper) {
  auto call_matrix = Message::NewTheoMatrix();
  call_matrix->option = call;
  call_matrix->lower = lower;
  call_matrix->upper = upper;
  auto put_matrix = Message::NewTheoMatrix();
  put_matrix->option = put;
  put_matrix->lower = lower;
  put_matrix->upper = upper;
  for (auto i = lower; i <= upper; ++i) {
    double s = call->HedgeUnderlying()->ConvertHalfToPrice(i);
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

    int idx = i - lower;
    call_matrix->theos[idx].spot = s;
    call_matrix->theos[idx].volatility = v;
    call_matrix->theos[idx].ss_rate = p.basis;
    call_matrix->theos[idx].theo = call_theo.theo;
    call_matrix->theos[idx].delta = call_theo.delta;
    call_matrix->theos[idx].gamma = call_theo.gamma;
    call_matrix->theos[idx].theta = call_theo.theta;
    put_matrix->theos[idx].spot = s;
    put_matrix->theos[idx].volatility = v;
    put_matrix->theos[idx].ss_rate = p.basis;
    put_matrix->theos[idx].theo = put_theo.theo;
    put_matrix->theos[idx].delta = put_theo.delta;
    put_matrix->theos[idx].gamma = put_theo.gamma;
    put_matrix->theos[idx].theta = put_theo.theta;
    // LOG_TRA << boost::format("Calculate %1%: s(%2%), v(%3%), r(%4%), q(%5%), t(%6%), "
    //     "theo(%7%), delta(%8%), gamma(%9%), theta(%10%)") % op->Id() % s % v % r % q %
    //   t % theo.theo % theo.delta % theo.gamma % theo.theta;
  }
  dm_->Publish(call_matrix);
  dm_->Publish(put_matrix);
}

void TheoCalculator::CalculateAndPublish(const Option *option,
                                         Parameters &p,
                                         double time_value,
                                         base::TickType lower,
                                         base::TickType upper) {
  auto matrix = Message::NewTheoMatrix();
  matrix->option = option;
  matrix->lower = lower;
  matrix->upper = upper;
  for (auto i = lower; i <= upper; ++i) {
    double s = option->HedgeUnderlying()->ConvertHalfToPrice(i);
    double k = option->Strike();
    double v = vol_model_->Calculate(*p.volatility, future_, s, k, p.rate, p.basis, time_value);

    Model::TheoData theo;
    bool call = option->CallPut() == Proto::OptionType::Call;
    if (future_) {
      model_->Calculate(call, s + p.basis, k, v, p.rate, p.rate, time_value, theo);
    } else {
      model_->Calculate(call, s, k, v, p.rate, p.basis, time_value, theo);
    }

    int idx = i - lower;
    matrix->theos[idx].spot = s;
    matrix->theos[idx].volatility = v;
    matrix->theos[idx].ss_rate = p.basis;
    matrix->theos[idx].theo = theo.theo;
    matrix->theos[idx].delta = theo.delta;
    matrix->theos[idx].gamma = theo.gamma;
    matrix->theos[idx].theta = theo.theta;
    // LOG_TRA << boost::format("Calculate %1%: s(%2%), v(%3%), r(%4%), q(%5%), t(%6%), "
    //     "theo(%7%), delta(%8%), gamma(%9%), theta(%10%)") % op->Id() % s % v % r % q %
    //   t % theo.theo % theo.delta % theo.gamma % theo.theta;
  }
  dm_->Publish(matrix);
}

// TheoMatrixPtr TheoCalculator::CalculateTheo(const Option* op, const std::shared_ptr<Parameter>
//     &param, base::TickType lower, base::TickType upper, double t)
// {
//   const Instrument *underlying = dm_->GetUnderlying();
//   if (unlikely(!param->rate))
//   {
//     LOG_WAN << boost::format("Can't get interest rate of %1% for %2%") %
//       to_iso_string(op->Maturity()) % underlying->Id();
//     return nullptr;
//   }
//   double r = *param->rate;
//   if (unlikely(!param->basis))
//   {
//     LOG_WAN << boost::format("Can't get ssrate of %1% for %2%") %
//       to_iso_string(op->Maturity()) % underlying->Id();
//     return nullptr;
//   }
//   double q = *param->basis;
//   if (unlikely(!param->volatility))
//   {
//     LOG_WAN << boost::format("Can't get volatility curve of %1% for %2%") %
//       to_iso_string(op->Maturity()) % underlying->Id();
//     return nullptr;
//   }
//   auto matrix = Message::NewTheoMatrix();
//   assert(op);
//   matrix->option = op;
//   bool future = underlying->Type() == Proto::InstrumentType::Future;
//   for (auto i = lower; i <= upper; ++i)
//   {
//     double s = underlying->ConvertHalfToPrice(i);
//     double v = vol_model_->Calculate(*param->volatility, future, s, op->Strike(), r, q, t);

//     Model::TheoData theo;
//     if (future)
//     {
//       model_->Calculate(op->CallPut() == Proto::OptionType::Call, s + q,
//           op->Strike(), v, r, r, t, theo);
//     }
//     else
//     {
//       model_->Calculate(op->CallPut() == Proto::OptionType::Call, s, op->Strike(), v, r, q, t, theo);
//     }

//     int idx = i - lower;
//     matrix->theos[idx].spot = s;
//     matrix->theos[idx].volatility = v;
//     matrix->theos[idx].ss_rate = q;
//     matrix->theos[idx].theo = theo.theo;
//     matrix->theos[idx].delta = theo.delta;
//     matrix->theos[idx].gamma = theo.gamma;
//     matrix->theos[idx].theta = theo.theta;
//     LOG_TRA << boost::format("Calculate %1%: s(%2%), v(%3%), r(%4%), q(%5%), t(%6%), "
//         "theo(%7%), delta(%8%), gamma(%9%), theta(%10%)") % op->Id() % s % v % r % q %
//       t % theo.theo % theo.delta % theo.gamma % theo.theta;
//   }
//   matrix->lower = lower;
//   matrix->upper = upper;
//   return std::move(matrix);
// }
