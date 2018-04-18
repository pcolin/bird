#ifndef MODEL_PRICE_H
#define MODEL_PRICE_H

#include "Message.h"
#include "Instrument.h"
#include "Price.pb.h"
#include "PricingSpec.pb.h"
#include <memory>

// namespace Proto
// {
//   class Price;
//   // enum UnderlyingTheoType;
// }

using base::PriceType;
using base::VolumeType;
using base::PRICE_UNDEFINED;
using base::VOLUME_UNDEFINED;

struct PriceLevel
{
  operator bool() const { return volume != VOLUME_UNDEFINED; }

  PriceType price = PRICE_UNDEFINED;
  VolumeType volume = VOLUME_UNDEFINED;
};

struct Price
{
  static const int LEVELS = 5;
  Price() : header(MsgType::Price) {}

  MsgHeader header;
  const Instrument *instrument;
  PriceLevel last;
  PriceLevel bids[LEVELS];
  PriceLevel asks[LEVELS];
  PriceType adjusted_price = PRICE_UNDEFINED;
  PriceType adjust = PRICE_UNDEFINED;
  PriceType open = PRICE_UNDEFINED;
  PriceType high = PRICE_UNDEFINED;
  PriceType low = PRICE_UNDEFINED;
  PriceType close = PRICE_UNDEFINED;
  PriceType pre_close = PRICE_UNDEFINED;
  PriceType pre_settlement = PRICE_UNDEFINED;
  PriceType amount = PRICE_UNDEFINED;
  VolumeType volume = VOLUME_UNDEFINED;

  std::string Dump() const;
  std::shared_ptr<Proto::Price> Serialize() const;
};

typedef std::shared_ptr<Price> PricePtr;

class UnderlyingPrice
{
public:
  UnderlyingPrice(const Instrument *underlying); /// : underlying_(underlying) {}

  void SetParameter(Proto::UnderlyingTheoType type, double elastic, double elastic_limit);
  void ApplyElastic(PricePtr &price, double delta);
  bool ApplyElastic(double delta);
  PriceType Get() const { return theo_; }

private:
  PriceType CalcTheo(const PricePtr &p) const;
  PriceType CalcBaryCentre(PriceLevel &bid, PriceLevel &ask) const;
  PriceType CalcDepthBary(const PricePtr &p) const;
  PriceType AdjustWithMidpoint(PriceType p, PriceType bid, PriceType ask) const;
  bool UseDepthBary(const PricePtr &p) const;

  const Instrument *underlying_;
  PricePtr price_;
  // int64_t theo_ = 0;
  PriceType theo_ = 0;
  PriceType adjust_ = 0;

  Proto::UnderlyingTheoType type_ = Proto::Midpoint;
  double elastic_ = 0;
  double elastic_limit_ = 0;

  std::mutex mtx_;
};

#endif
