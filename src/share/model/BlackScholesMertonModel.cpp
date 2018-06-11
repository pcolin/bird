#include <iostream>
#include <cmath>
#include "BlackScholesMertonModel.h"

namespace Model
{

void BlackScholesMertonModel::Calculate(bool call, double s, double k, double t, double r, double q,
    double v, GreeksData &theo)
{
  rt_ = std::exp(-r * t);
  qt_ = std::exp(-q * t);
  sqrt_t_ = std::sqrt(t);
  double d1 = GetD1(s, k, v, r, q, t);
  double d2 = GetD2FromD1(d1, v);
  N_d1_ = NormalCumDist(d1);
  N_d2_ = NormalCumDist(d2);
  N_neg_d1_ = NormalCumDist(-d1);
  N_neg_d2_ = NormalCumDist(-d2);
  D_d1_ = NormalProbabilityDensity(d1);
  theo.spot = s;
  theo.vol_theta = CalculateVolTheta(s, v) / AnnualTradingDays;
  if (call)
  {
    theo.theo = CalculateCall(s, k);
    theo.delta = CalculateCallDelta();
    theo.rate_theta = CalculateCallRateTheta(s, k, r, q) / AnnualTradingDays;
    theo.rho1 = CalculateCallRho1(k, t);
    theo.rho2 = CalculateCallRho2(s, t);
  }
  else
  {
    theo.theo = CalculatePut(s, k);
    theo.delta = CalculatePutDelta();
    theo.rate_theta = CalculatePutRateTheta(s, k, r, q) / AnnualTradingDays;
    theo.rho1 = CalculatePutRho1(k, t);
    theo.rho2 = CalculatePutRho2(s, t);
  }
  theo.theta = theo.vol_theta + theo.rate_theta;
  theo.gamma = CalculateGamma(s, v);
  theo.vega = CalculateVega(s) / 100;
}

void BlackScholesMertonModel::Calculate(bool call, double s, double k, double v, double r, double q,
    double t, TheoData &theo)
{
  rt_ = std::exp(-r * t);
  qt_ = std::exp(-q * t);
  sqrt_t_ = std::sqrt(t);
  double d1 = GetD1(s, k, v, r, q, t);
  double d2 = GetD2FromD1(d1, v);
  N_d1_ = NormalCumDist(d1);
  N_d2_ = NormalCumDist(d2);
  N_neg_d1_ = NormalCumDist(-d1);
  N_neg_d2_ = NormalCumDist(-d2);
  D_d1_ = NormalProbabilityDensity(d1);
  if (call)
  {
    theo.theo = CalculateCall(s, k);
    theo.delta = CalculateCallDelta();
    theo.theta = CalculateCallTheta(s, k, v, r, q);
  }
  else
  {
    theo.theo = CalculatePut(s, k);
    theo.delta = CalculatePutDelta();
    theo.theta = CalculatePutTheta(s, k, v, r, q);
  }
  theo.gamma = CalculateGamma(s, v);
}

double BlackScholesMertonModel::CalculateDelta(bool call, double s, double k, double v, double r,
    double q, double t)
{
  qt_ = std::exp(-q * t);
  sqrt_t_ = std::sqrt(t);
  double d1 = GetD1(s, k, v, r, q, t);
  if (call)
  {
    N_d1_ = NormalCumDist(d1);
    return CalculateCallDelta();
  }
  else
  {
    N_neg_d1_ = NormalCumDist(-d1);
    return CalculatePutDelta();
  }
}

double BlackScholesMertonModel::CalculateIV(bool call, double v, double p, double s, double k,
    double r, double q, double t)
{
  double min_vol = 0;
  double guess = PricingModel::Calculate(call, s, k, min_vol, r, q, t);
  if (p > guess)
  {
    double max_vol = 3;
    guess = PricingModel::Calculate(call, s, k, max_vol, r, q, t);
    if (p < guess)
    {
      double a = min_vol, b = max_vol, mid = v;
      const int max_iterators = 15;
      for (int i = 0; i < max_iterators; ++i)
      {
        guess = PricingModel::Calculate(call, s, k, mid, r, q, t);
        if (guess > p)
          b = mid;
        else
          a = mid;
        mid = (a + b) / 2;
      }
      return mid;
    }
    else
      return max_vol;
  }
  else
    return min_vol;
}

double BlackScholesMertonModel::CalculateCall(double s, double k)
{
  return s * qt_ * N_d1_ - (k * rt_) * N_d2_;
}

double BlackScholesMertonModel::CalculatePut(double s, double k)
{
  return k * rt_ * N_neg_d2_ - s * qt_ * N_neg_d1_;
}

double BlackScholesMertonModel::GetD1(double s, double k, double v, double r, double q, double t)
{
  return (std::log(s / k) + (r - q + (v * v * 0.5)) * t) / (v * sqrt_t_);
}

double BlackScholesMertonModel::GetD2(double s, double k, double v, double r, double q, double t)
{
  return GetD1(s, k, v, r, q, t) - v * sqrt_t_;
}

double BlackScholesMertonModel::GetD2FromD1(double d1, double v)
{
  return d1 - v * sqrt_t_;
}

double BlackScholesMertonModel::NormalProbabilityDensity(double d1)
{
  return std::exp(-d1 * d1 * 0.5) / 2.506628274631; /// std::sqrt(PI*2)
}

double BlackScholesMertonModel::CalculateVega(double s)
{
  return qt_ * sqrt_t_ * s * D_d1_;
}

double BlackScholesMertonModel::CalculateCallDelta()
{
  return qt_ * N_d1_;
}

double BlackScholesMertonModel::CalculatePutDelta()
{
  return -qt_ * N_neg_d1_;
}

double BlackScholesMertonModel::CalculateGamma(double s, double v)
{
  return qt_ * D_d1_ / (s * v * sqrt_t_);
}

double BlackScholesMertonModel::CalculateCallTheta(double s, double k, double v, double r, double q)
{
  return -qt_ * s * D_d1_ * v / (2.0 * sqrt_t_) - r * k * rt_ * N_d2_ + q * s * qt_ * N_d1_;
}

double BlackScholesMertonModel::CalculatePutTheta(double s, double k, double v, double r, double q)
{
  return -qt_ * s * D_d1_ * v / (2.0 * sqrt_t_) + r * k * rt_ * N_neg_d2_ - q * s * qt_ * N_neg_d1_;
}

double BlackScholesMertonModel::CalculateVolTheta(double s, double v)
{
  return -qt_ * s * D_d1_ * v / (2.0 * sqrt_t_);
}

double BlackScholesMertonModel::CalculateCallRateTheta(double s, double k, double r, double q)
{
  return -r * k * rt_ * N_d2_ + q * s * qt_ * N_d1_;
}

double BlackScholesMertonModel::CalculatePutRateTheta(double s, double k, double r, double q)
{
  return r * k * rt_ * N_neg_d2_ - q * s * qt_ * N_neg_d1_;
}

double BlackScholesMertonModel::CalculateCallRho1(double k, double t)
{
  return k * t * rt_ * N_d2_;
}

double BlackScholesMertonModel::CalculateCallRho2(double s, double t)
{
  return -s * t * qt_ * N_d1_;
}

double BlackScholesMertonModel::CalculatePutRho1(double k, double t)
{
  return -k * t * rt_ * N_neg_d2_;
}

double BlackScholesMertonModel::CalculatePutRho2(double s, double t)
{
  return s * t * qt_ * N_neg_d1_;
}

}
