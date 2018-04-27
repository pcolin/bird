#include <cmath>
#include "PricingModel.h"

namespace Model
{

double PricingModel::Calculate(bool call, double s, double k, double v, double r, double q, double t)
{
  double tmp = v * std::sqrt(t);
  double d1 = (std::log(s / k) + (r - q + v * v * 0.5) * t) / tmp;
  double d2 = d1 - tmp;
  double ret = s * std::exp(-q * t) * NormalCumDist(d1) - k * std::exp(-r * t) * NormalCumDist(d2);
  return call ? ret : (ret - s * std::exp(-q * t) + k * std::exp(-r * t));
}

double PricingModel::CalculateIV(bool call, double v, double p, double s, double k,
    double r, double q, double t)
{
  double min_vol = 0;
  double guess = Calculate(call, s, k, min_vol, r, q, t);
  if (p > guess)
  {
    double max_vol = 3;
    guess = Calculate(call, s, k, max_vol, r, q, t);
    if (p < guess)
    {
      double a = min_vol, b = max_vol, mid = v;
      const int max_iterators = 15;
      for (int i = 0; i < max_iterators; ++i)
      {
        guess = Calculate(call, s, k, mid, r, q, t);
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

double PricingModel::NormalCumDist(double x)
{
  double cn = 0;
  double xabs = std::abs(x);
  if (xabs <= 37)
  {
    double exponential = std::exp(-std::pow(xabs, 2) / 2);
    if (xabs < 10 * 0.707106781186548) /// std::sqrt(0.5) = 0.707106781186548
    {
      double build = 3.52624965998911E-02 * xabs + 0.700383064443688;
      build = build * xabs + 6.37396220353165;
      build = build * xabs + 33.912866078383;
      build = build * xabs + 112.079291497871;
      build = build * xabs + 221.213596169931;
      build = build * xabs + 220.206867912376;
      cn = exponential * build;
      build = 8.83883476483184E-02 * xabs + 1.75566716318264;
      build = build * xabs + 16.064177579207;
      build = build * xabs + 86.7807322029461;
      build = build * xabs + 296.564248779674;
      build = build * xabs + 637.333633378831;
      build = build * xabs + 793.826512519948;
      build = build * xabs + 440.413735824752;
      cn = cn / build;
    }
    else
    {
      double build = xabs + 0.65;
      build = xabs + 4.0 / build;
      build = xabs + 3.0 / build;
      build = xabs + 2.0 / build;
      build = xabs + 1.0 / build;
      cn = exponential / build / 2.506628274631;
    }
  }

  return (x > 0) ? (1 - cn) : cn;
}

}
