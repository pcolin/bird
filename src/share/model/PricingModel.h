#ifndef MODEL_PRICING_MODEL_H
#define MODEL_PRICING_MODEL_H

namespace Model
{

// const double PRICE_UNDEFINED = 0;

struct GreeksData
{
  double spot;
  double theo;
  double delta;
  double gamma;
  double theta;
  double vega;
  double rho1;
  double rho2;
  double vol_theta;
  double rate_theta;
};

struct TheoData
{
  double theo;
  double delta;
  double gamma;
  double theta;
};

class PricingModel
{
public:
  static const int AnnualTradingDays = 245;

  virtual double Calculate(bool call, double s, double k, double v, double r, double q, double t);
  virtual void Calculate(bool call, double s, double k, double v, double r, double q, double t,
      GreeksData &theo) = 0;
  virtual void Calculate(bool call, double s, double k, double v, double r, double q, double t,
      TheoData &theo) = 0;
  virtual double CalculateDelta(bool call, double s, double k, double v, double r, double q,
      double t) = 0;
  virtual double CalculateIV(bool call, double v, double p, double s, double k, double r, double q,
      double t);

  virtual const char* Name() const  = 0;

protected:
  double NormalCumDist(double x);
};

}

#endif
