#include "CoxRossRubinsteinModel.h"

#include <cmath>
#include <algorithm>
// #include "../vol_library/MMVolCalculator.h"

namespace Model
{

CoxRossRubinsteinModel::CoxRossRubinsteinModel(int depth)
  : depth_(depth), st_(new double[(depth + 2) * (depth + 1) / 2]),
    pt_(new double[depth * (depth + 1) / 2]),
    st2_(new double[(depth + 1) * (2 * depth + 1)]),
    pt2_(new double[(2 * depth + 1) * depth])
{}

double CoxRossRubinsteinModel::Calculate(bool call, double s, double k, double v, double r,
    double q, double t)
{
  InitTree(st_, pt_, depth_, call, s, k, v, r, t);
  InitTree(st2_, pt2_, 2 * depth_, call, s, k, v, r, t);
  return 2 * pt2_[0] - pt_[0];
}

void CoxRossRubinsteinModel::Calculate(bool call, double s, double k, double v, double r,
    double q, double t, GreeksData &theo)
{
  InitTree(st_, pt_, depth_, call, s, k, v, r, t);
  InitTree(st2_, pt2_, 2 * depth_, call, s, k, v, r, t);
  theo.spot = s;
  theo.theo = 2 * pt2_[0] - pt_[0];
  theo.delta = 2 * DeltaFromTree(pt2_, st2_) - DeltaFromTree(pt_, st_);
  theo.theta = (2 * (pt2_[4] - pt2_[0]) / (t / depth_) - (pt_[4] - pt_[0]) / (2 * t / depth_)) /
    PricingModel::AnnualTradingDays;
  theo.gamma = 2 * GammaFromTree(pt2_, st2_) - GammaFromTree(pt_, st_);
  theo.vega = Calculate(call, s, k, v + 0.005, r, 0, t) - Calculate(call, s, k, v - 0.005, r, 0, t);
  theo.rho1 = (Calculate(call, s, k, v, r + 0.005, 0, t) -
      Calculate(call, s, k, v, r - 0.005, 0, t)) * 100;
  theo.rho2 = 0;
  theo.vol_theta = (Calculate(call, s, k, v, 0, 0, 0.99 * t) - Calculate(call, s, k, v, 0, 0, t)) /
    (0.01 * t) / PricingModel::AnnualTradingDays;
  theo.rate_theta = theo.theta - theo.vol_theta;
}

void CoxRossRubinsteinModel::Calculate(bool call, double s, double k, double v, double r,
    double q, double t, TheoData &theo)
{
  InitTree(st_, pt_, depth_, call, s, k, v, r, t);
  InitTree(st2_, pt2_, 2 * depth_, call, s, k, v, r, t);
  theo.theo = 2 * pt2_[0] - pt_[0];
  theo.delta = 2 * DeltaFromTree(pt2_, st2_) - DeltaFromTree(pt_, st_);
  // theo.theta = (2 * (pt2_[4] - pt2_[0]) / (t / depth_) - (pt_[4] - pt_[0]) / (2 * t / depth_)) /
  //   PricingModel::AnnualTradingDays;
  // theo.gamma = 2 * GammaFromTree(pt2_, st2_) - GammaFromTree(pt_, st_);
}

double CoxRossRubinsteinModel::CalculateDelta(bool call, double s, double k, double v, double r,
    double q, double t)
{
  InitTree(st_, pt_, depth_, call, s, k, v, r, t);
  InitTree(st2_, pt2_, 2 * depth_, call, s, k, v, r, t);
  return 2 * DeltaFromTree(pt2_, st2_) - DeltaFromTree(pt_, st_);
}
// void CoxRossRubinsteinModel::Calculate(CalculateParameter &para,
//                                      VolLibrary::VolCurveParam &vcp,
//                                      double tick, int depth,
//                                      std::vector<double> &prices,
//                                      std::vector<double> &deltas) {
//   prices.resize(2 * depth + 1);
//   deltas.resize(2 * depth + 1);
//
//   int single_depth = std::floor(std::min(10.0, para.spot / 200) / tick);
//   int iter_num =
//       1 +
//       2 * std::max(0.0, std::ceil((depth - single_depth / 2.0) / single_depth));
//   int mid_iter_num = (iter_num - 1) / 2;
//   para.spot -= mid_iter_num * (2 * single_depth + 1) * tick;
//   SimpleTheoData theo;
//   for (int i = 0; i < iter_num; ++i) {
//     VolLibrary::MMVolCalculator volCalc;
//     double vol = 0;
//     if (volCalc.calcVol(vcp, para.spot, para.strike, para.rate, para.ss_rate, 0,
//                         para.maturity, true, vol)) {
//       Calculate(para.call, para.spot, para.strike, para.maturity, para.rate,
//                 para.ss_rate, vol, true, true,theo);
//       for (int j = -single_depth; j <= single_depth; ++j) {
//         int idx = (i - mid_iter_num) * (2 * single_depth + 1) + depth + j;
//         if (idx >= 0 && idx <= 2 * depth) {
//           double diff = j * tick;
//           prices[idx] =
//               theo.theo + theo.delta * diff + 0.5 * diff * diff * theo.gamma;
//           deltas[idx] = theo.delta + theo.gamma * diff;
//         }
//       }
//       para.spot += (2 * single_depth + 1) * tick;
//     }
//   }
// }
//void CoxRossRubinsteinModel::Calculate(CalculateParameter &para,
//                                     VolLibrary::VolCurveParam &vcp,
//                                     double tick, int depth,
//                                     std::vector<double> &prices,
//                                     std::vector<double> &deltas) {
//  int size = 2 * depth + 1;
//  prices.resize(size);
//  deltas.resize(size);
//
//  double spot = para.spot - depth * tick;
//  VolLibrary::MMVolCalculator volCalc;
//  double vol = 0;
//  for (int i = 0; i < size; ++i) {
//    if (volCalc.calcVol(vcp, spot, para.strike, para.rate, para.ss_rate, 0, para.maturity, true,
//                        vol)) {
//      InitTree(st_, pt_, depth_, spot, para.ss_rate, para.strike, para.rate,
//               para.maturity, vol, para.call);
//      InitTree(st_, pt2_, 2 * depth_, spot, para.ss_rate, para.strike, para.rate,
//               para.maturity, vol, para.call);
//      prices[i] = 2 * pt2_[0] - pt_[0];
//      deltas[i] = 2 * DeltaFromTree(pt2_, st_) -
//                  DeltaFromTree(pt_, st_);
//    }
//    spot += tick;
//  }
//}
//
//void CoxRossRubinsteinModel::Calculate(CalculateParameter &para,
//                                     VolLibrary::VolCurveParam &vcp,
//                                     double lower, double upper, double tick,
//                                     std::vector<double> &spots,
//                                     std::vector<double> &prices,
//                                     std::vector<double> &deltas) {
//  int size = (upper - lower) / tick + 1;
//  spots.resize(size);
//  prices.resize(size);
//  deltas.resize(size);
//
//  VolLibrary::MMVolCalculator volCalc;
//  double vol = 0;
//  for (int i = 0; i < size; ++i) {
//    spots[i] = lower + i * tick;
//    if (volCalc.calcVol(vcp, spots[i], para.strike, para.rate, para.ss_rate, 0, para.maturity,
//                        true, vol)) {
//      InitTree(st_, pt_, depth_, spots[i], para.ss_rate, para.strike, para.rate,
//               para.maturity, vol, para.call);
//      InitTree(st_, pt2_, 2 * depth_, spots[i], para.ss_rate, para.strike, para.rate,
//               para.maturity, vol, para.call);
//      prices[i] = 2 * pt2_[0] - pt_[0];
//      deltas[i] = 2 * DeltaFromTree(pt2_, st_) -
//                  DeltaFromTree(pt_, st_);
//    }
//  }
//}
// void CoxRossRubinsteinModel::Calculate(CalculateParameter &para,
//                                      VolLibrary::VolCurveParam &vcp,
//                                      double lower, double upper, double tick,
//                                      std::vector<double> &spots,
//                                      std::vector<double> &prices,
//                                      std::vector<double> &deltas) {
//   int size = (upper - lower) / tick + 1;
//   spots.resize(size);
//   prices.resize(size);
//   deltas.resize(size);
//
//   for (int i = 0; i < size; ++i) {
//     spots[i] = lower + i * tick;
//   }
//
//   int single_depth = std::floor(std::min(10.0, (lower + upper) / 400) / tick);
//   int single_size = 1 + 2 * single_depth;
//   int iter_num = std::ceil(double(size) / single_size);
//
//   double spot = lower + single_depth * tick;
//   if (single_size >= size) {
//     spot = lower + ((size - 1) / 2) * tick;
//   }
//
//   SimpleTheoData theo;
//   for (int i = 0; i < iter_num; ++i) {
//     VolLibrary::MMVolCalculator volCalc;
//     double vol = 0;
//     if (volCalc.calcVol(vcp, spot, para.strike, para.rate, para.ss_rate, 0,
//                         para.maturity, true, vol)) {
//       Calculate(para.call, spot, para.strike, para.maturity, para.rate,
//                 para.ss_rate, vol, true,true, theo);
//       for (int j = -single_depth; j <= single_depth; ++j) {
//         int idx = (spot - lower) / tick + j;
//         if (idx >= 0 && idx <= size - 1) {
//           double diff = j * tick;
//           prices[idx] =
//               theo.theo + theo.delta * diff + 0.5 * diff * diff * theo.gamma;
//           deltas[idx] = theo.delta + theo.gamma * diff;
//         }
//       }
//       spot += single_size * tick;
//     }
//   }
// }

//void CoxRossRubinsteinModel::Calculate(CalculateParameter &para,
//                                     VolLibrary::VolCurveParam &vcp,
//                                     double lower, double upper, double tick,
//                                     std::vector<TheoData> &theoDataSet) {
//  std::vector<double> prices;
//  std::vector<double> deltas;
//  std::vector<double> spots;
//  Calculate(para, vcp, lower, upper, tick, spots, prices, deltas);
//
//  theoDataSet.resize(spots.size());
//  for (auto i = 0u; i < spots.size(); ++i) {
//    theoDataSet[i].refPrice = spots[i];
//    theoDataSet[i].theo = prices[i];
//    theoDataSet[i].delta = deltas[i];
//  }
//}
//
//double CoxRossRubinsteinModel::CalculateSStar(bool call, double spot,
//                                            double strike, double maturity,
//                                            double rate, double ss_rate,
//                                            double vol) {
//  double precision = 0.00001 * spot;
//  if (call) {
//    double p = Calculate(call, spot, strike, maturity, rate, ss_rate, vol, true,true);
//    if (std::abs(p - (spot + ss_rate - strike)) < precision)
//      return spot;
//
//    const double multiplier = 3;
//    double a = spot, b = multiplier * spot, mid;
//    const int max_iterators = 15;
//    for (int i = 0; i < max_iterators; ++i) {
//      mid = 0.5 * (a + b);
//      p = Calculate(call, mid, strike, maturity, rate, ss_rate, vol, true,true);
//      if (std::abs(p - (mid + ss_rate - strike)) < precision)
//        b = mid;
//      else
//        a = mid;
//    }
//    return mid;
//  } else {
//    double p = Calculate(call, spot, strike, maturity, rate, ss_rate, vol, true,true);
//    if (std::abs(p - (strike - spot - ss_rate)) < precision)
//      return spot;
//
//    double a = 0, b = spot, mid;
//    const int max_iterators = 15;
//    for (int i = 0; i < max_iterators; ++i) {
//      mid = 0.5 * (a + b);
//      p = Calculate(call, mid, strike, maturity, rate, ss_rate, vol, true,true);
//      if (std::abs(p - (strike - mid - ss_rate)) < precision)
//        a = mid;
//      else
//        b = mid;
//    }
//    return mid;
//  }
//}

void CoxRossRubinsteinModel::InitTree(array_ptr &st, array_ptr &pt, int depth,
    bool call, double s, double k, double v, double r, double t)
{
  double dt = t / depth;
  double u = std::exp(std::sqrt(dt) * v);
  double d = 1 / u;
  double u2 = u * u;

  st[0] = s;
  for (int i = 1; i <= depth; ++i)
  {
    st[i * (i + 1) / 2] = st[(i - 1) * i / 2] * d;
    for (int j = 1; j <= i; ++j)
    {
      st[i * (i + 1) / 2 + j] = st[i * (i + 1) / 2 + j - 1] * u2;
    }
  }

  for (int i = 0; i < depth; ++i)
  {
    pt[(depth - 1) * depth / 2 + i] =
      PricingModel::Calculate(call, st[(depth - 1) * depth / 2 + i], k, v, 0, 0, dt);
  }

  double p = (1 - d) / (u - d);
  double discount = std::exp(-r * dt);
  if (call)
  {
    for (int i = depth - 2; i >= 0; --i)
    {
      for (int j = 0; j < i + 1; ++j)
      {
        double tmp = discount * (p * pt[(i + 1) * (i + 2) / 2 + j + 1] +
            (1 - p) * pt[(i + 1) * (i + 2) / 2 + j]);
        pt[i * (i + 1) / 2 + j] = std::max(st[i * (i + 1) / 2 + j] - k, tmp);
      }
    }
  }
  else
  {
    for (int i = depth - 2; i >= 0; --i)
    {
      for (int j = 0; j < i + 1; ++j)
      {
        double tmp = discount * (p * pt[(i + 1) * (i + 2) / 2 + j + 1] +
            (1 - p) * pt[(i + 1) * (i + 2) / 2 + j]);
        pt[i * (i + 1) / 2 + j] = std::max(k - st[i * (i + 1) / 2 + j], tmp);
      }
    }
  }
}

double CoxRossRubinsteinModel::DeltaFromTree(array_ptr &pt, array_ptr &st)
{
  return (pt[2] - pt[1]) / (st[2] - st[1]);
}

double CoxRossRubinsteinModel::GammaFromTree(array_ptr &pt, array_ptr &st)
{
  double d1 = (pt[5] - pt[4]) / (st[5] - st[4]);
  double d2 = (pt[4] - pt[3]) / (st[4] - st[3]);
  double ds = 0.5 * (st[5] - st[3]);
  return (d1 - d2) / ds;
}

}
