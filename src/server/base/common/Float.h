#ifndef BASE_FLOAT_H
#define BASE_FLOAT_H

namespace base
{
  const double EPS = 1E-8; /// epsilon

  inline bool IsEqual(double p1, double p2)
  {
    return fabs(p1 - p2) < EPS;
  }

  inline bool IsLessThan(double p1, double p2)
  {
    return p1 < p2 - EPS;
  }

  inline bool IsMoreThan(double p1, double p2)
  {
    return p1 > p2 + EPS;
  }

  inline bool IsLessOrEqual(double p1, double p2)
  {
    return p1 < p2 + EPS;
  }

  inline bool IsMoreOrEqual(double p1, double p2)
  {
    return p1 > p2 - EPS;
  }

  inline bool IsBetween(double p, double down, double up)
  {
    return IsMoreOrEqual(p, down) && IsLessOrEqual(p, up);
  }
}

#endif
