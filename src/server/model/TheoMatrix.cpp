#include "TheoMatrix.h"
#include <iostream>
#include "base/common/Likely.h"
#include "base/common/Float.h"
#include "base/logger/Logging.h"

void TheoData::InterpolateFrom(const TheoData &td1, const TheoData &td2) {
  assert(base::IsBetween(spot, td1.spot, td2.spot));
  if (base::IsEqual(td1.spot, td2.spot) == false) {
    auto ratio = (spot - td1.spot) / (td2.spot - td1.spot);
    theo = td1.theo + ratio * (td2.theo - td1.theo);
    delta = td1.delta + ratio * (td2.delta - td1.delta);
    gamma = td1.gamma + ratio * (td2.gamma - td1.gamma);
    theta = td1.theta + ratio * (td2.theta - td1.theta);
  } else {
    *this = td1;
  }
}

bool TheoMatrix::FindTheo(base::PriceType spot, TheoData &theo) const {
  auto tick = option->HedgeUnderlying()->ConvertToHalfTick(spot);
  if (unlikely(base::IsEqual((double)tick,
                             option->HedgeUnderlying()->ConvertToHalfTickRatio(spot)))) {
    auto idx = tick - lower;
    // LOG_DBG << tick << " " << idx;
    if (likely(idx >= 0 && idx < 2 * DEPTH + 1 && theos[idx])) {
      theo = theos[idx];
      return true;
    }
  } else {
    auto idx1 = tick - lower;
    auto idx2 = idx1 + 1;
    // LOG_DBG << tick << " " << idx1 << " " << lower << theos[idx2].spot;
    if (likely(idx1 >= 0 && idx2 < 2 * DEPTH + 1 && theos[idx2])) {
      theo.spot = spot;
      theo.volatility = theos[idx1].volatility;
      theo.ss_rate = theos[idx1].ss_rate;
      theo.InterpolateFrom(theos[idx1], theos[idx2]);
      return true;
    }
  }
  return false;
}

// std::string TheoMatrix::Dump()
// {
//   // assert (option);
//   if (option)
//   {
//   std::string id = option->Id();
//   // LOG_DBG << "Begin to dump " << id;
//   LOG_DBG << "Begin to dump " << option->Id();
//   std::ostringstream oss;
//   // LOG_DBG << "before to dump ";
//   LOG_DBG << "before to dump " << option->Id();
//   // oss << boost::format("%1% lower(%2%) upper(%3%)") % option->Id() % lower % upper;
//   LOG_DBG << "Middle to dump " << option->Id();
//   for (int i = 0; (i < 2 * DEPTH + 1) && theos[i]; ++i)
//   {
//     oss << " [" << i << '|' << theos[i].spot << '|' << theos[i].volatility << '|' << theos[i].ss_rate
//       << '|' << theos[i].theo << '|' << theos[i].delta << '|' << theos[i].gamma << '|'
//       << theos[i].theta << "]";
//   }
//   if (oss.good())
//   {
//     // LOG_DBG << "End to dump " << option->Id() << " : " << oss.str().size();
//     return oss.str();
//   }
//   else
//   {
//     LOG_ERR << "Err stream";
//     return "Err Stream";
//   }
//   // LOG_DBG << "End to dump " << option->Id();
//   }
//   else
//   {
//     LOG_DBG << "option is null";
//   }
// }

namespace base {

LogStream& operator<<(LogStream& stream, const TheoMatrixPtr &matrix) {
  stream << matrix->option->Id() << " lower(" << matrix->lower << ") upper(" << matrix->upper << ')';
  for (int i = 0; (i < 2 * matrix->DEPTH + 1) && matrix->theos[i]; ++i) {
    // double s = matrix->theos[i].spot;
    // double v = matrix->theos[i].volatility;
    // double ssr = matrix->theos[i].ss_rate;
    // double t = matrix->theos[i].theo;
    // double d = matrix->theos[i].delta;
    // double g = matrix->theos[i].gamma;
    // double th = matrix->theos[i].theta;
    // stream << " [" << i << '|' << s << '|' << v << '|' << ssr << '|' << t << '|' << d << '|' << g << '|' << th << "]";
    stream << " [" << i << '|' << matrix->theos[i].spot << '|' << matrix->theos[i].volatility << '|'
           << matrix->theos[i].ss_rate << '|' << matrix->theos[i].theo << '|'
           << matrix->theos[i].delta << '|' << matrix->theos[i].gamma << '|'
           << matrix->theos[i].theta << "]";
  }
  return stream;
}

} // namespace base
