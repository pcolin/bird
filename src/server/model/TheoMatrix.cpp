#include "TheoMatrix.h"
#include "base/common/Likely.h"
#include "base/common/Float.h"
#include "boost/format.hpp"

void TheoData::InterpolateFrom(const TheoData &td1, const TheoData &td2)
{
  assert(base::IsBetween(spot, td1.spot, td2.spot));
  if (base::IsEqual(td1.spot, td2.spot) == false)
  {
    auto ratio = (spot - td1.spot) / (td2.spot - td1.spot);
    theo = td1.theo + ratio * (td2.theo - td1.theo);
    delta = td1.delta + ratio * (td2.delta - td1.delta);
    gamma = td1.gamma + ratio * (td2.gamma - td1.gamma);
    theta = td1.theta + ratio * (td2.theta - td1.theta);
  }
  else
  {
    *this = td1;
  }
}

bool TheoMatrix::FindTheo(base::PriceType spot, TheoData &theo) const
{
  auto tick = option->HedgeUnderlying()->ConvertToHalfTick(spot);
  if (unlikely(base::IsEqual((double)tick, option->HedgeUnderlying()->ConvertToHalfTickRatio(spot))))
  {
    auto idx = tick - lower;
    if (likely(idx >= 0 && idx < 2 * DEPTH + 1 && theos[idx]))
    {
      theo = theos[idx];
      return true;
    }
  }
  else
  {
    auto idx1 = tick - lower;
    auto idx2 = idx1 + 1;
    if (likely(idx1 >= 0 && idx2 < 2 * DEPTH + 1 && theos[idx2]))
    {
      theo.spot = spot;
      theo.InterpolateFrom(theos[idx1], theos[idx2]);
      return true;
    }
  }
  return false;
}

std::string TheoMatrix::Dump() const
{
  std::ostringstream oss;
  oss << boost::format("%1% lower(%1%) upper(%2%)") % option->Id() % lower % upper;
  for (int i = 0; (i < 2 * DEPTH + 1) && theos[i]; ++i)
  {
    oss << " [" << i << '-' << theos[i].spot << '-' << theos[i].volatility << '-' << theos[i].ss_rate
      << '-' << theos[i].theo << '-' << theos[i].delta << '-' << theos[i].gamma << '-'
      << theos[i].theta << "]";
  }
  return oss.str();
}
