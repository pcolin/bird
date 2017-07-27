#ifndef MODEL_INSTRUMENT_H
#define MODEL_INSTRUMENT_H

#include <string>
#include "base/common/Types.h"

enum class InstrumentType : int8_t
{
  Stock = 0,
  Future = 1,
  Option = 2,
};

enum class InstrumentStatus : int8_t
{
  Unkown = 0,
  Closed = 1,
  PreOpen = 2,
  OpeningAuction = 3,
  Trading = 4,
  Fuse = 5,
  Auction = 6,
  ClosingAuction = 7,
  Halt = 8,
};

enum class Currencies : int8_t
{
  CNY = 0,
  HKD = 1,
  USD = 2,
  EUR = 3,
  JPY = 4,
  SGD = 5,
};

enum class Exchanges : int8_t
{
  SH, /// SSE
  SZ, /// SZSE
  CF, /// CFFEX
  SF, /// SHFE
  DE, /// DCE
  ZE, /// CZCE
};

class Instrument
{
  public:
    Instrument(InstrumentType type) : type_(type) {}

    const std::string& Id() const { return id_; }
    void Id(const std::string& id) { id_ = id; }

    const std::string& Symbol() const { return symbol_; }
    void Symbol(const std::string& symbol) { symbol_ = symbol; }

    const Exchanges Exchange() const { return exchange_; }
    void Exchange(Exchanges exchange) { exchange_ = exchange; }

    const InstrumentType Type() const { return type_; }
    void Type(InstrumentType type) { type_ = type; }

    const InstrumentStatus Status() const { return status_; }
    void Status(InstrumentStatus status) { status_ = status; }

    const Currencies Currency() const { return currency_; }
    void Currency(Currencies currency) { currency_ = currency; }

    const base::PriceType Tick() const { return tick_; }
    void Tick(base::PriceType tick) { tick_ = tick; }

    const base::PriceType Multiplier() const { return multiplier_; }
    void Multiplier(base::PriceType multiplier) { multiplier_ = multiplier; }

    const base::PriceType Highest() const { return highest_; }
    void Highest(base::PriceType highest) { highest_ = highest; }

    const base::PriceType Lowest() const { return lowest_; }
    void Lowest(base::PriceType lowest) { lowest_ = lowest; }

    const Instrument* Underlying() const { return underlying_; }
    void Underlying(const Instrument* underlying) { underlying_ = underlying; }

    const Instrument* HedgeUnderlying() const { return hedge_underlying_; }
    void HedgeUnderlying(const Instrument* hedge_undl) { hedge_underlying_ = hedge_undl; }

  private:
    std::string id_;
    std::string symbol_;
    Exchanges exchange_;
    InstrumentType type_;
    InstrumentStatus status_;
    Currencies currency_;
    base::PriceType tick_ = base::PRICE_UNDEFINED;
    base::PriceType multiplier_ = base::PRICE_UNDEFINED;
    base::PriceType highest_ = base::PRICE_UNDEFINED;
    base::PriceType lowest_ = base::PRICE_UNDEFINED;

    const Instrument* underlying_ = nullptr;
    const Instrument* hedge_underlying_ = nullptr;
};

#endif
