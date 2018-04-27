#ifndef MODEL_THEO_MATRIX_H
#define MODEL_THEO_MATRIX_H

#include "Message.h"
#include "Option.h"

struct TheoData
{
  operator bool() const { return spot != base::PRICE_UNDEFINED; }

  base::PriceType spot = base::PRICE_UNDEFINED;
  double volatility = 0.0;
  double ss_rate = 0.0;
  double theo = 0.0;
  double delta = 0.0;
  double gamma = 0.0;
  double theta = 0.0;
  // double vega = 0.0;
  // double rho = 0.0;
};

struct TheoMatrix
{
  static const int DEPTH = 30;

  TheoMatrix() : header(MsgType::TheoMatrix) {}
  std::string Dump() const;

  MsgHeader header;
  // char pricing_name[16] = {0};
  const Option *option = nullptr;
  TheoData theo[2 * DEPTH + 1];
  base::TickType lower;
  base::TickType upper;
};

typedef std::shared_ptr<TheoMatrix> TheoMatrixPtr;

#endif
