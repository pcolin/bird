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

// using namespace boost::gregorian;

TheoCalculator::TheoCalculator(const std::string &name, DeviceManager *dm)
  : visitor_(this), name_(name), dm_(dm)
{}

void TheoCalculator::Start()
{
  auto spec = ClusterManager::GetInstance()->FindPricingSpec(name_);
  if (spec)
  {
    if (Initialize(spec))
    {
      running_ = true;
      thread_ = std::make_unique<std::thread>([&]()
        {
          LOG_INF << "Start calculator thread...";
          while (running_)
          {
            CalculatorEvent e;
            if (events_.try_dequeue(e))
            {
              boost::apply_visitor(visitor_, e);
            }
          }
        });
    }
  }
  else
  {
    LOG_ERR << boost::format("Pricing %1% is not existed") % name_;
  }
}

void TheoCalculator::Stop()
{
  running_ = false;
  if (thread_)
  {
    thread_->join();
    thread_.reset();
  }
}

bool TheoCalculator::Initialize(const std::shared_ptr<Proto::PricingSpec> &spec)
{
  Proto::PricingModel model = spec->model();
  LOG_INF << boost::format("Start running %1%: model(%2%)") % spec->name() %
    Proto::PricingModel_Name(model);
  if (model == Proto::PricingModel::BSM)
  {
    model_ = std::make_shared<Model::BlackScholesMertonModel>();
  }
  else if (model == Proto::PricingModel::CRR)
  {
    model_ = std::make_shared<Model::CoxRossRubinsteinModel>();
  }
  else
  {
    LOG_ERR << "Unknown pricing model";
    return false;
  }
  vol_model_ = std::make_shared<Model::VolatilityModel>();
  interval_ = spec->interval() * base::MILLION;
  // spot_ = base::PRICE_UNDEFINED;
  parameters_.clear();
  for (auto &op : spec->options())
  {
    auto *option = base::down_cast<const Option*>(ProductManager::GetInstance()->FindId(op));
    if (option)
    {
      assert (option->HedgeUnderlying() == dm_->GetUnderlying());
      auto param = std::make_shared<Parameter>();
      auto &maturity = option->Maturity();
      auto *pm = ParameterManager::GetInstance();
      double tmp = 0;
      if (pm->GetInterestRate(maturity, tmp))
      {
        param->rate = std::make_shared<double>(tmp);
      }
      else if (param->rate)
      {
        param->rate.reset();
      }

      if (pm->GetSSRate(option->HedgeUnderlying(), maturity, tmp))
      {
        param->basis = std::make_shared<double>(tmp);
      }
      else if (param->basis)
      {
        param->basis.reset();
      }

      auto vc = pm->GetVolatilityCurve(option->HedgeUnderlying(), maturity);
      if (vc)
      {
        param->volatility = std::make_shared<Model::VolatilityModel::Parameter>();
        param->volatility->spot = vc->spot();
        param->volatility->atm_vol = vc->atm_vol();
        param->volatility->call_convex = vc->call_convex();
        param->volatility->put_convex = vc->put_convex();
        param->volatility->call_slope = vc->call_slope();
        param->volatility->put_slope = vc->put_slope();
        param->volatility->call_cutoff = vc->call_cutoff();
        param->volatility->put_cutoff = vc->put_cutoff();
        param->volatility->vcr = vc->vcr();
        param->volatility->scr = vc->scr();
        param->volatility->ccr = vc->ccr();
        param->volatility->spcr = vc->spcr();
        param->volatility->sccr = vc->sccr();
      }
      else if (param->volatility)
      {
        param->volatility.reset();
      }

      parameters_.emplace(option, param);
      LOG_DBG << boost::format("%1%: Add %2%") % spec->name() % op;
    }
    else
    {
      LOG_ERR << "Failed to find option " << op;
    }
  }
  return true;
}

void TheoCalculator::OnPrice(const PricePtr &p)
{
  p->header.SetInterval(0);
  LOG_INF << "OnPrice : " << p->Dump();
  base::TickType tick = p->instrument->ConvertToHalfTick(p->adjusted_price);
  if (tick < lower_ || tick > upper_)
  {
    auto begin_time = base::Now();
    lower_ = std::max(1, tick - TheoMatrix::DEPTH);
    upper_ = tick + TheoMatrix::DEPTH;

    std::map<boost::gregorian::date, double> time_values;
    for(auto &it : parameters_)
    {
      double t = 0;
      auto &maturity = it.first->Maturity();
      auto tv = time_values.find(maturity);
      if (tv != time_values.end())
      {
        t = tv->second;
      }
      else
      {
        auto exch = ParameterManager::GetInstance()->GetExchange();
        if (exch)
        {
          t = exch->GetTimeValue(it.first);
          time_values.emplace(maturity, t);
        }
        else
        {
          LOG_WAN << "Can't Get exchange parameter for " << it.first->Id();
          continue;
        }
      }
      LOG_DBG << "Begin to calculate " << it.first->Id();
      auto matrix = CalculateTheo(it.first, it.second, lower_, upper_, t);
      if (matrix)
      {
        matrix->header.interval[0] = p->header.interval[0];
        matrix->header.SetInterval(1);
        matrix->header.time = p->header.time;
        dm_->Publish(matrix);
      }
    }
    calculate_time_ = base::Now();
    LOG_INF << boost::format("Recalc theo with spot(%1%), matrix[%2%, %3%] finished : %4%us") %
      p->adjusted_price % lower_ % upper_ % (calculate_time_ - begin_time);
  }
  spot_ = p->adjusted_price;
}

void TheoCalculator::OnTrade(const TradePtr &trade)
{
  const Option* op = base::down_cast<const Option*>(trade->instrument);
  auto it = parameters_.find(op);
  if (it != parameters_.end() && spot_ != base::PRICE_UNDEFINED)
  {
    LOG_INF << "Recalculate theo due to trade : " << trade->Dump();
    auto exch = ParameterManager::GetInstance()->GetExchange();
    if (exch)
    {
      double time_value = exch->GetTimeValue(op);
      auto tick = dm_->GetUnderlying()->ConvertToHalfTick(spot_);
      base::TickType lower = std::max(1, tick - TheoMatrix::DEPTH);
      base::TickType upper = tick + TheoMatrix::DEPTH;
      LOG_DBG << boost::format("Recalc theo with spot(%1%), matrix[%2%, %3%]") %
        spot_ % lower % upper;
      auto matrix = CalculateTheo(op, it->second, lower, upper, time_value);
      if (matrix)
      {
        dm_->Publish(matrix);
      }
    }
    else
    {
      LOG_WAN << "Can't Get exchange parameter for " << op->Id();
    }
  }
}

void TheoCalculator::OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &h)
{
  if (base::Now() - calculate_time_ > interval_)
  {
    LOG_INF << "Recalcuate timely";
    Recalculate();
  }
}

void TheoCalculator::OnPricingSpec(const std::shared_ptr<Proto::PricingSpec> &spec)
{
  assert(spec->name() == name_);
  if (Initialize(spec))
  {
    LOG_INF << "Recalculate due to PricingSpec update";
    Recalculate();
  }
}

void TheoCalculator::OnExchangeParameter(const std::shared_ptr<Proto::ExchangeParameterReq> &req)
{
  LOG_INF << "Recalculate due to exchange parameter update";
  Recalculate();
}

void TheoCalculator::OnInterestRate(const std::shared_ptr<Proto::InterestRateReq> &req)
{
  auto *pm = ParameterManager::GetInstance();
  for(auto &it : parameters_)
  {
    double rate = 0;
    if (pm->GetInterestRate(it.first->Maturity(), rate))
    {
      if (it.second->rate)
      {
        *it.second->rate = rate;
      }
      else
      {
        it.second->rate = std::make_shared<double>(rate);
      }
    }
    else
    {
      it.second->rate.reset();
    }
  }

  LOG_INF << "Recalculate due to interest rate update";
  Recalculate();
}

void TheoCalculator::OnSSRateReq(const std::shared_ptr<Proto::SSRateReq> &req)
{
  auto *pm = ParameterManager::GetInstance();
  for(auto &it : parameters_)
  {
    double basis = 0;
    if (pm->GetSSRate(it.first->HedgeUnderlying(), it.first->Maturity(), basis))
    {
      if (it.second->basis)
      {
        *it.second->basis = basis;
      }
      else
      {
        it.second->basis = std::make_shared<double>(basis);
      }
    }
    else if (it.second->basis)
    {
      it.second->basis.reset();
    }
  }

  LOG_INF << "Recalculate theo due to SSRate update";
  Recalculate();
}

void TheoCalculator::OnVolatilityCurve(const std::shared_ptr<Proto::VolatilityCurveReq> &req)
{
  auto *pm = ParameterManager::GetInstance();
  for(auto &it : parameters_)
  {
    auto vc = pm->GetVolatilityCurve(it.first->HedgeUnderlying(), it.first->Maturity());
    if (vc)
    {
      if (!it.second->volatility)
      {
        it.second->volatility = std::make_shared<Model::VolatilityModel::Parameter>();
      }
      it.second->volatility->spot = vc->spot();
      it.second->volatility->atm_vol = vc->atm_vol();
      it.second->volatility->call_convex = vc->call_convex();
      it.second->volatility->put_convex = vc->put_convex();
      it.second->volatility->call_slope = vc->call_slope();
      it.second->volatility->put_slope = vc->put_slope();
      it.second->volatility->call_cutoff = vc->call_cutoff();
      it.second->volatility->put_cutoff = vc->put_cutoff();
      it.second->volatility->vcr = vc->vcr();
      it.second->volatility->scr = vc->scr();
      it.second->volatility->ccr = vc->ccr();
      it.second->volatility->spcr = vc->spcr();
      it.second->volatility->sccr = vc->sccr();
    }
    else if (it.second->volatility)
    {
      it.second->volatility.reset();
    }
  }

  LOG_INF << "Recalculate theo due to volatility curve update";
  Recalculate();
}

void TheoCalculator::Recalculate()
{
  if (spot_ != base::PRICE_UNDEFINED)
  {
    auto begin_time = base::Now();
    auto tick = dm_->GetUnderlying()->ConvertToHalfTick(spot_);
    lower_ = std::max(1, tick - TheoMatrix::DEPTH);
    upper_ = tick + TheoMatrix::DEPTH;

    std::map<boost::gregorian::date, double> time_values;
    for(auto &p : parameters_)
    {
      double t = 0;
      auto &maturity = p.first->Maturity();
      auto it = time_values.find(maturity);
      if (it != time_values.end())
      {
        t = it->second;
      }
      else
      {
        auto exch = ParameterManager::GetInstance()->GetExchange();
        if (exch)
        {
          t = exch->GetTimeValue(p.first);
          time_values.emplace(maturity, t);
        }
        else
        {
          LOG_WAN << "Can't Get exchange parameter for " << p.first->Id();
          continue;
        }
      }
      auto matrix = CalculateTheo(p.first, p.second, lower_, upper_, t);
      if (matrix)
      {
        matrix->header.SetTime();
        dm_->Publish(matrix);
      }
    }
    calculate_time_ = base::Now();
    LOG_INF << boost::format("Recalc theo with spot(%1%), matrix[%2%, %3%] finished : %4%us") %
      spot_ % lower_ % upper_ % (calculate_time_ - begin_time);
  }
}

TheoMatrixPtr TheoCalculator::CalculateTheo(const Option* op, const std::shared_ptr<Parameter>
    &param, base::TickType lower, base::TickType upper, double t)
{
  const Instrument *underlying = dm_->GetUnderlying();
  if (unlikely(!param->rate))
  {
    LOG_WAN << boost::format("Can't get interest rate of %1% for %2%") %
      to_iso_string(op->Maturity()) % underlying->Id();
    return nullptr;
  }
  double r = *param->rate;
  if (unlikely(!param->basis))
  {
    LOG_WAN << boost::format("Can't get ssrate of %1% for %2%") %
      to_iso_string(op->Maturity()) % underlying->Id();
    return nullptr;
  }
  double q = *param->basis;
  if (unlikely(!param->volatility))
  {
    LOG_WAN << boost::format("Can't get volatility curve of %1% for %2%") %
      to_iso_string(op->Maturity()) % underlying->Id();
    return nullptr;
  }
  auto matrix = Message::NewTheoMatrix();
  matrix->option = op;
  for (auto i = lower; i <= upper; ++i)
  {
    double s = underlying->ConvertHalfToPrice(i);

    bool future = underlying->Type() == Proto::InstrumentType::Future;
    double v = vol_model_->Calculate(*param->volatility, future, s, op->Strike(), r, q, t);

    Model::TheoData theo;
    if (future)
    {
      model_->Calculate(op->CallPut() == Proto::OptionType::Call, s + q,
          op->Strike(), v, r, r, t, theo);
    }
    else
    {
      model_->Calculate(op->CallPut() == Proto::OptionType::Call, s, op->Strike(), v, r, q, t, theo);
    }

    int idx = i - lower;
    matrix->theos[idx].volatility = v;
    matrix->theos[idx].ss_rate = q;
    matrix->theos[idx].theo = theo.theo;
    matrix->theos[idx].delta = theo.delta;
    matrix->theos[idx].gamma = theo.gamma;
    matrix->theos[idx].theta = theo.theta;
    LOG_DBG << boost::format("Calculate %1%: s(%2%), v(%3%), r(%4%), q(%5%), t(%6%), "
        "theo(%7%), delta(%8%), gamma(%9%), theta(%10%)") % op->Id() % s % v % r % q %
      t % theo.theo % theo.delta % theo.gamma % theo.theta;
  }
  matrix->lower = lower;
  matrix->upper = upper;
  return matrix;
}
