#include "TheoMatrix.h"
#include "boost/format.hpp"

std::string TheoMatrix::Dump() const
{
  std::ostringstream oss;
  oss << boost::format("%1% lower(%1%) upper(%2%)") % option->Id() % lower % upper;
  for (int i = 0; (i < 2 * DEPTH + 1) && theo[i]; ++i)
  {
    oss << " [" << i << '-' << theo[i].spot << '-' << theo[i].volatility << '-' << theo[i].ss_rate
      << '-' << theo[i].theo << '-' << theo[i].delta << '-' << theo[i].gamma << '-' << theo[i].theta 
      << "]";
  }
  return oss.str();
}
