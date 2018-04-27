#ifndef MODEL_COX_ROSS_RUBINSTEIN_MODEL_H_
#define MODEL_COX_ROSS_RUBINSTEIN_MODEL_H_

#include <vector>
#include <memory>
#include "PricingModel.h"
// #include "../vol_library/VolModel.h"

namespace Model
{

class CoxRossRubinsteinModel : public PricingModel
{
  typedef std::unique_ptr<double[]> array_ptr;

public:
  const char* Name() const { return "CRR"; }

  CoxRossRubinsteinModel(int depth = 20);

  double Calculate(bool call, double s, double k, double v, double r, double q, double t) override;
  void Calculate(bool call, double s, double k, double v, double r, double q, double t,
      GreeksData &result) override;
  void Calculate(bool call, double s, double k, double v, double r, double q, double t,
      TheoData &result) override;

  //void Calculate(CalculateParameter &para, VolLibrary::VolCurveParam &vcp,
  //               double tick, int depth, std::vector<double> &prices,
  //               std::vector<double> &deltas);
  //void Calculate(CalculateParameter &para, VolLibrary::VolCurveParam &vcp,
  //               double lower, double upper, double tick,
  //               std::vector<double> &spots, std::vector<double> &prices,
  //               std::vector<double> &deltas);
  //void Calculate(CalculateParameter &para, VolLibrary::VolCurveParam &vcp,
  //               double lower, double upper, double tick,
  //               std::vector<TheoData> &theoDataSet);

  //double CalculateSStar(bool call, double spot, double strike, double maturity,
  //                      double rate, double ss_rate, double vol);

private:
  void InitTree(array_ptr &st, array_ptr &pt, int depth,
      bool call, double s, double k, double v, double r, double t);
  double DeltaFromTree(array_ptr &pt, array_ptr &st);
  double GammaFromTree(array_ptr &pt, array_ptr &st);

  int depth_;
  array_ptr st_;
  array_ptr pt_;
  array_ptr st2_;
  array_ptr pt2_;
};

}

#endif
