#ifndef MODEL_VOLATILITY_MODEL_H
#define MODEL_VOLATILITY_MODEL_H

namespace Model
{

class VolatilityModel
{
public:
  struct Parameter
  {
    double spot;
    double skew;
    double atm_vol;
    double call_convex;
    double put_convex;
    double call_slope;
    double put_slope;
    double call_cutoff;
    double put_cutoff;
    double vcr;
    double scr;
    double ccr;
    double spcr;
    double sccr;
  };

public:
  double Calculate(const Parameter &param, bool future, double s, double k, double r, double q,
      double t);

private:
  double Integral(double put_cutoff, double put_slope, double call_cutoff, double call_slope,
      double k);
};

}

#endif
