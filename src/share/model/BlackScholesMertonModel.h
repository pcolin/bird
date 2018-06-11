#ifndef MODEL_BLACK_SCHOLES_MERTON_MODEL_H_
#define MODEL_BLACK_SCHOLES_MERTON_MODEL_H_

#include "PricingModel.h"

namespace Model
{

class BlackScholesMertonModel : public PricingModel
{
public:
  const char* Name() const { return "BSM"; }

  void Calculate(bool call, double s, double k, double v, double r, double q, double t,
      GreeksData &theo) override;
  void Calculate(bool call, double s, double k, double v, double r, double q, double t,
      TheoData &theo) override;
  double CalculateDelta(bool call, double s, double k, double v, double r, double q,
      double t) override;
  double CalculateIV(bool call, double v, double p,
      double s, double k, double r, double q, double t) override;

private:
  double CalculateCall(double s, double k);
  double CalculatePut(double s, double k);
  double GetD1(double s, double k, double v, double r, double q, double t);
  double GetD2(double s, double k, double v, double r, double q, double t);
  double GetD2FromD1(double d1, double v);
  double NormalProbabilityDensity(double d1);
  double CalculateVega(double s);
  double CalculateCallDelta();
  double CalculatePutDelta();
  double CalculateGamma(double s, double v);
  double CalculateCallTheta(double s, double k, double v, double r, double q);
  double CalculatePutTheta(double s, double k, double v, double r, double q);
  double CalculateVolTheta(double s, double v);
  double CalculateCallRateTheta(double s, double k, double r, double q);
  double CalculatePutRateTheta(double s, double k, double r, double q);
  double CalculateCallRho1(double k, double t);
  double CalculateCallRho2(double s, double t);
  double CalculatePutRho1(double k, double t);
  double CalculatePutRho2(double s, double t);

private:
  double rt_;
  double qt_;
  double sqrt_t_ = 0;
  double N_d1_ = 0;
  double N_d2_ = 0;
  double N_neg_d1_ = 0;
  double N_neg_d2_ = 0;
  double D_d1_ = 0;
};

}
#endif
