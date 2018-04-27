#include "VolatilityModel.h"
#include <cmath>

namespace Model
{

double VolatilityModel::Calculate(const Parameter &param, bool future, double s, double k, double r,
    double q, double t)
{
  const double EPS = 1e-8;
  if (s < EPS) return -1;

  const double MIN_TIME_VALUE = 0.5 / 245;
  if (t < MIN_TIME_VALUE) t = MIN_TIME_VALUE;

  double skew = param.skew;
  double atm_vol = param.atm_vol;
  double call_convex = param.call_convex;
  double put_convex = param.put_convex;
  double call_slope = param.call_slope;
  double put_slope = param.put_slope;
  if (s * param.spot > 0)
  {
    double moving = std::log(s / param.spot) / std::sqrt(t);
    skew += param.scr * moving;
    atm_vol += param.vcr * (-skew * 0.5) * moving;
    call_convex += param.ccr * moving;
    put_convex += param.ccr * moving;
    call_slope += param.sccr * moving;
    put_slope += param.spcr * moving;
  }

  double ks = 0;
  if (future)
  {
    ks = std::log(k / (s + q));
  }
  else
  {
    ks = std::log(k / (s * std::exp((r - q) * t)));
  }

  double nu = 0;
  if (ks > EPS)
    nu = call_convex;
  else if (ks < -EPS)
    nu = put_convex;
  else
    nu = (call_convex + put_convex) * 0.5;

  nu = std::sqrt(skew * skew + nu * nu);
  double rho = -skew  / nu;

  double x = Integral(param.put_cutoff, put_slope, param.call_cutoff, call_slope, ks);
  double z = -nu / std::sqrt(t) * x / atm_vol;
  double denom = std::log((std::sqrt(1 - 2 * rho * z + z * z) + z - rho) / (1 - rho));
  if (std::abs(denom) < EPS)
  {
    return atm_vol;
  }
  else
  {
    return -nu / std::sqrt(t) * ks / denom;
  }
}

double VolatilityModel::Integral(double put_cutoff, double put_slope, double call_cutoff,
    double call_slope, double k)
{
  static const int COUNT = 10;
  static const double pt[COUNT] = { -0.9739065285, -0.8650633667, -0.6794095683, -0.4333953941,
    -0.148874339, 0.148874339, 0.4333953941, 0.6794095683, 0.8650633667, 0.9739065285 };
  static const double weight[COUNT] = { 0.0666713443, 0.1494513492, 0.2190863625, 0.2692667193,
    0.2955242247, 0.2955242247, 0.2692667193, 0.2190863625, 0.1494513492, 0.0666713443 };

  double sum = 0;
  for (int i = 0; i < COUNT; ++i)
  {
    double x = (pt[i] + 1) * k * 0.5;
    double fct = 1;
    if (x > call_cutoff)
    {
      fct = std::exp(call_slope * (x - call_cutoff));
    }
    else if (x < put_cutoff)
    {
      fct = std::exp(put_slope * (x - put_cutoff));
    }
    sum += weight[i] / fct;
  }
  return sum * k * 0.5;
}

}
