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

TheoCalculator::TheoCalculator(const std::string &name, DeviceManager *dm)
  : visitor_(this), name_(name), dm_(dm)
{}

void TheoCalculator::Start()
{
  auto spec = ClusterManager::GetInstance()->FindPricer(name_);
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

bool TheoCalculator::Initialize(const std::shared_ptr<Proto::Pricer> &spec)
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
  auto *underlying = dm_->GetUnderlying();
  auto options = ProductManager::GetInstance()->FindInstruments([&](const Instrument *inst)
      { return inst->Type() == Proto::InstrumentType::Option &&
               inst->HedgeUnderlying() == underlying; });
  for (auto *op : options)
  {
    auto *option = base::down_cast<const Option*>(op);
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

      if (pm->GetSSRate(underlying, maturity, tmp))
      {
        param->basis = std::make_shared<double>(tmp);
      }
      else if (param->basis)
      {
        param->basis.reset();
      }

      auto vc = pm->GetVolatilityCurve(underlying, maturity);
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
      else/// if (param->volatility)
      {
        param->volatility.reset();
        LOG_WAN << boost::format("Volatility curve of %1% for %2% is null") %
          to_iso_string(maturity) % underlying->Id();
      }

      auto it = parameters_.find(option->Maturity());
      if (it == parameters_.end())
      {
        it = parameters_.emplace(option->Maturity(), ParameterMap()).first;
      }
      it->second.emplace(option, param);
      LOG_DBG << boost::format("%1%: Add %2%") % spec->name() % op->Id();
    }
    else
    {
      LOG_ERR << "Failed to find option " << op;
    }
  }
  return true;
}

void TheoCalculator::OnPrice(const PricePtr &price)
{
  price->header.SetInterval(0);
  LOG_DBG << price;
  tick_ = price->instrument->ConvertToHalfTick(price->adjusted_price);
  if (tick_ < lower_ || tick_ > upper_)
  {
    auto exch = ParameterManager::GetInstance()->GetExchange();
    if (unlikely(!exch))
    {
      LOG_WAN << "No exchange parameter";
      return;
    }

    auto begin_time = base::Now();
    SetLowerUpper(lower_, upper_);
    for (auto &it : parameters_)
    {
      double t = exch->GetTimeValue(it.first);
      for (auto &p : it.second)
      {
        auto matrix = CalculateTheo(p.first, p.second, lower_, upper_, t);
        if (matrix)
        {
          matrix->header.interval[0] = price->header.interval[0];
          matrix->header.SetInterval(1);
          matrix->header.time = price->header.time;
          dm_->Publish(matrix);
        }
      }
    }

    calculate_time_ = base::Now();
    LOG_INF << boost::format("Recalc theo with spot(%1%), matrix[%2%, %3%] finished : %4%us") %
      price->adjusted_price % lower_ % upper_ % (calculate_time_ - begin_time);
  }
}

void TheoCalculator::OnTrade(const TradePtr &trade)
{
  if (trade->instrument->Type() == Proto::InstrumentType::Option)
  {
    const Option* op = base::down_cast<const Option*>(trade->instrument);
    auto it = parameters_.find(op->Maturity());
    if (it != parameters_.end())
    {
      auto p = it->second.find(op);
      if (p != it->second.end() && tick_)
      {
        LOG_INF << "Recalculate theo due to trade : " << trade;
        auto exch = ParameterManager::GetInstance()->GetExchange();
        if (exch)
        {
          double t = exch->GetTimeValue(it->first);
          base::TickType lower = 0, upper = 0;
          SetLowerUpper(lower, upper);
          auto matrix = CalculateTheo(op, p->second, lower, upper, t);
          if (matrix)
          {
            dm_->Publish(matrix);
          }
        }
        else
        {
          LOG_WAN << "No exchange parameter";
        }
      }
    }
  }
}

void TheoCalculator::OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &h)
{
  if (base::Now() - calculate_time_ > interval_ && tick_)
  {
    int64_t time = RecalculateAll();
    LOG_INF << boost::format("Timely recalc with matrix[%1%, %2%] finished: %3%us")
      % lower_ % upper_ % time;
  }
}

void TheoCalculator::OnPricer(const std::shared_ptr<Proto::Pricer> &spec)
{
  assert(spec->name() == name_);
  if (Initialize(spec) && tick_)
  {
    int64_t time = RecalculateAll();
    LOG_INF << boost::format("Update Pricer to recalc with matrix[%1%, %2%] finished: %3%us")
      % lower_ % upper_ % time;
  }
}

void TheoCalculator::OnExchangeParameter(const std::shared_ptr<Proto::ExchangeParameterReq> &req)
{
  if (tick_)
  {
    int64_t time = RecalculateAll();
    LOG_INF << boost::format("Update ExchangeParameter to recalc with matrix[%1%, %2%] "
        "finished: %3%us") % lower_ % upper_ % time;
  }
}

void TheoCalculator::OnInterestRate(const std::shared_ptr<Proto::InterestRateReq> &req)
{
  for(auto &it : parameters_)
  {
    for (auto &p : it.second)
    {
      double rate = 0;
      if (ParameterManager::GetInstance()->GetInterestRate(it.first, rate))
      {
        if (p.second->rate)
        {
          *p.second->rate = rate;
        }
        else
        {
          p.second->rate = std::make_shared<double>(rate);
        }
      }
      else
      {
        p.second->rate.reset();
      }
    }
  }

  if (tick_)
  {
    int64_t time = RecalculateAll();
    LOG_INF << boost::format("Update InterestRate to recalc with matrix[%1%, %2%] finished: %3%us") %
      lower_ % upper_ % time;
  }
}

void TheoCalculator::OnSSRate(const std::shared_ptr<Proto::SSRate> &ssr)
{
  auto maturity = boost::gregorian::from_undelimited_string(ssr->maturity());
  auto it = parameters_.find(maturity);
  if (it != parameters_.end())
  {
    for (auto &p : it->second)
    {
      if (p.second->basis)
      {
        *p.second->basis = ssr->rate();
      }
      else
      {
        p.second->basis = std::make_shared<double>(ssr->rate());
      }
    }
    if (tick_)
    {
      LOG_INF << boost::format("Update SSRate of %1%@%2% to recalc") %
        ssr->underlying() % ssr->maturity();
      base::TickType lower = 0, upper = 0;
      SetLowerUpper(lower, upper);
      Recalculate(maturity, it->second, lower, upper);
    }
  }
}

void TheoCalculator::OnVolatilityCurve(const std::shared_ptr<Proto::VolatilityCurve> &vc)
{
  auto maturity = boost::gregorian::from_undelimited_string(vc->maturity());
  auto it = parameters_.find(maturity);
  if (it != parameters_.end())
  {
    for (auto &p : it->second)
    {
      if (!p.second->volatility)
      {
        p.second->volatility = std::make_shared<Model::VolatilityModel::Parameter>();
      }
      p.second->volatility->spot = vc->spot();
      p.second->volatility->atm_vol = vc->atm_vol();
      p.second->volatility->call_convex = vc->call_convex();
      p.second->volatility->put_convex = vc->put_convex();
      p.second->volatility->call_slope = vc->call_slope();
      p.second->volatility->put_slope = vc->put_slope();
      p.second->volatility->call_cutoff = vc->call_cutoff();
      p.second->volatility->put_cutoff = vc->put_cutoff();
      p.second->volatility->vcr = vc->vcr();
      p.second->volatility->scr = vc->scr();
      p.second->volatility->ccr = vc->ccr();
      p.second->volatility->spcr = vc->spcr();
      p.second->volatility->sccr = vc->sccr();
    }
    if (tick_)
    {
      LOG_INF << boost::format("Update VolatilityCurve of %1%@%2% to recalc") %
        vc->underlying() % vc->maturity();
      base::TickType lower = 0, upper = 0;
      SetLowerUpper(lower, upper);
      Recalculate(maturity, it->second, lower, upper);
    }
  }
}

int64_t TheoCalculator::RecalculateAll()
{
  auto begin_time = base::Now();
  SetLowerUpper(lower_, upper_);
  for (auto &it : parameters_)
  {
    Recalculate(it.first, it.second, lower_, upper_);
  }
  calculate_time_ = base::Now();
  return calculate_time_ - begin_time;
}

void TheoCalculator::Recalculate(const boost::gregorian::date &maturity, ParameterMap &parameters,
    base::TickType lower, base::TickType upper)
{
  auto exch = ParameterManager::GetInstance()->GetExchange();
  if (exch)
  {
    double t = exch->GetTimeValue(maturity);
    for(auto &p : parameters)
    {
      auto matrix = CalculateTheo(p.first, p.second, lower, upper, t);
      if (matrix)
      {
        matrix->header.SetTime();
        dm_->Publish(matrix);
      }
    }
  }
  else
  {
    LOG_WAN << "No exchange parameter";
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
  assert(op);
  matrix->option = op;
  bool future = underlying->Type() == Proto::InstrumentType::Future;
  for (auto i = lower; i <= upper; ++i)
  {
    double s = underlying->ConvertHalfToPrice(i);
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
    matrix->theos[idx].spot = s;
    matrix->theos[idx].volatility = v;
    matrix->theos[idx].ss_rate = q;
    matrix->theos[idx].theo = theo.theo;
    matrix->theos[idx].delta = theo.delta;
    matrix->theos[idx].gamma = theo.gamma;
    matrix->theos[idx].theta = theo.theta;
    LOG_TRA << boost::format("Calculate %1%: s(%2%), v(%3%), r(%4%), q(%5%), t(%6%), "
        "theo(%7%), delta(%8%), gamma(%9%), theta(%10%)") % op->Id() % s % v % r % q %
      t % theo.theo % theo.delta % theo.gamma % theo.theta;
  }
  matrix->lower = lower;
  matrix->upper = upper;
  return std::move(matrix);
}
