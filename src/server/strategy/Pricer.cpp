#include "Pricer.h"
#include "BlackScholesMertonModel.h"
#include "CoxRossRubinsteinModel.h"
#include "ClusterManager.h"
#include "base/common/Likely.h"
#include "model/ProductManager.h"
#include "model/ParameterManager.h"
#include "base/logger/Logging.h"
#include "exchange/manager/ExchangeManager.h"

#include "boost/format.hpp"

using namespace boost::gregorian;

Pricer::Pricer(const std::string &name, DeviceManager *dm)
  : Strategy(name, dm)
{
  dispatcher_.RegisterCallback<Proto::PricingSpec>(
      std::bind(&Pricer::OnPricingSpec, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::InterestRateReq>(
      std::bind(&Pricer::OnInterestRateReq, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::SSRateReq>(
      std::bind(&Pricer::OnSSRateReq, this, std::placeholders::_1));
  dispatcher_.RegisterCallback<Proto::VolatilityCurveReq>(
      std::bind(&Pricer::OnVolatilityCurveReq, this, std::placeholders::_1));
}

void Pricer::OnStart()
{
  auto pricing = ClusterManager::GetInstance()->FindPricingSpec(name_);
  if (pricing)
  {
    Initialize(pricing);
  }
  else
  {
    LOG_ERR << "Failed to get pricing " << name_;
  }
}

void Pricer::OnStop()
{
  LOG_INF << "OnStop";
}

void Pricer::Initialize(const std::shared_ptr<Proto::PricingSpec> pricing)
{
  Proto::PricingModel model = pricing->model();
  LOG_INF << boost::format("Start running %1%: model(%2%)") % name_ %
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
    return;
  }
  vol_model_ = std::make_shared<Model::VolatilityModel>();
  spot_ = base::PRICE_UNDEFINED;
  option_parameters_.clear();
  for (auto &op : pricing->options())
  {
    auto *option = base::down_cast<const Option*>(ProductManager::GetInstance()->FindId(op));
    if (option)
    {
      assert (option->HedgeUnderlying() == dm_->GetUnderlying());
      auto param = std::make_shared<Parameter>();
      auto &date = option->Maturity();
      auto *pm = ParameterManager::GetInstance();
      double tmp = 0;
      if (pm->GetInterestRate(date, tmp))
      {
        param->rate = std::make_shared<double>(tmp);
      }
      if (pm->GetSSRate(dm_->GetUnderlying(), date, tmp))
      {
        param->basis = std::make_shared<double>(tmp);
      }
      auto vc = pm->GetVolatilityCurve(dm_->GetUnderlying(), date);
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
      option_parameters_.emplace(option, param);
      LOG_DBG << boost::format("%1%: Add %2%") % name_ % op;
    }
    else
    {
      LOG_ERR << "Failed to find option " << op;
    }
  }
}

void Pricer::OnPrice(const PricePtr &price)
{
  if (price->instrument == dm_->GetUnderlying())
  {
    price->header.SetInterval(0);
    LOG_INF << "OnPrice : " << price->Dump();
    base::TickType tick = price->instrument->ConvertToHalfTick(price->adjusted_price);
    if (tick < lower_ || tick > upper_)
    {
      lower_ = std::max(1, tick - TheoMatrix::DEPTH);
      upper_ = tick + TheoMatrix::DEPTH;
      LOG_DBG << boost::format("Recalc theo with spot(%1%), matrix[%2%, %3%]") %
        price->adjusted_price % lower_ % upper_;

      std::map<boost::gregorian::date, double> time_values;
      for(auto &p : option_parameters_)
      {
        double t = 0;
        auto &maturity = p.first->Maturity();
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
            t = exch->GetTimeValue(p.first);
            time_values.emplace(maturity, t);
          }
          else
          {
            LOG_WAN << "Can't Get exchange parameter for " << p.first->Id();
            continue;
          }
        }
        LOG_DBG << "Begin to calculate " << p.first->Id();
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
    spot_ = price->adjusted_price;
  }
}

void Pricer::OnTrade(const TradePtr &trade)
{
  const Option* op = base::down_cast<const Option*>(trade->instrument);
  auto it = option_parameters_.find(op);
  if (it != option_parameters_.end() && spot_ != base::PRICE_UNDEFINED)
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

bool Pricer::OnPricingSpec(const std::shared_ptr<Proto::PricingSpec> &msg)
{
  if (msg->name() == name_)
  {
    LOG_INF << "Restart pricer due to PricingSpec: " << msg->ShortDebugString();
    Initialize(msg);
  }
  return true;
}

bool Pricer::OnInterestRateReq(const std::shared_ptr<Proto::InterestRateReq> &req)
{
  LOG_INF << "Recalculate theo due to InterestRate: " << req->ShortDebugString();
  if (spot_ != base::PRICE_UNDEFINED)
  {
    auto tick = dm_->GetUnderlying()->ConvertToHalfTick(spot_);
    lower_ = std::max(1, tick - TheoMatrix::DEPTH);
    upper_ = tick + TheoMatrix::DEPTH;
    LOG_DBG << boost::format("Recalc theo with spot(%1%), matrix[%2%, %3%]") %
      spot_ % lower_ % upper_;

    std::map<boost::gregorian::date, double> time_values;
    for(auto &p : option_parameters_)
    {
      double t = 0;
      auto &maturity = p.first->Maturity();
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
  }
  return true;
}

bool Pricer::OnSSRateReq(const std::shared_ptr<Proto::SSRateReq> &req)
{
  return true;
}

bool Pricer::OnVolatilityCurveReq(const std::shared_ptr<Proto::VolatilityCurveReq> &req)
{
  return true;
}

TheoMatrixPtr Pricer::CalculateTheo(const Option* op, const std::shared_ptr<Parameter> &param,
    base::TickType lower, base::TickType upper, double t)
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
      model_->Calculate(op->CallPut() == Proto::OptionType::Call, s,
          op->Strike(), v, r, q, t, theo);
    }

    int idx = i - lower;
    matrix->theo[idx].volatility = v;
    matrix->theo[idx].ss_rate = q;
    matrix->theo[idx].theo = theo.theo;
    matrix->theo[idx].delta = theo.delta;
    matrix->theo[idx].gamma = theo.gamma;
    matrix->theo[idx].theta = theo.theta;
    LOG_DBG << boost::format("Calculate %1%: s(%2%), v(%3%), r(%4%), q(%5%), t(%6%), "
        "theo(%7%), delta(%8%), gamma(%9%), theta(%10%)") % op->Id() % s % v % r % q %
      t % theo.theo % theo.delta % theo.gamma % theo.theta;
  }
  matrix->lower = lower;
  matrix->upper = upper;
  return matrix;
}
