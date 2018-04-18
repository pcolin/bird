#ifndef MODEL_INSTRUMENT_H
#define MODEL_INSTRUMENT_H

#include <string>
#include <atomic>
#include "base/common/Types.h"
#include "Instrument.pb.h"
#include <boost/date_time/gregorian/gregorian.hpp>

class Instrument
{
  public:
    Instrument(Proto::InstrumentType type) : type_(type) {}

    const std::string& Id() const { return id_; }
    void Id(const std::string& id) { id_ = id; }

    const std::string& Symbol() const { return symbol_; }
    void Symbol(const std::string& symbol) { symbol_ = symbol; }

    const Proto::Exchange Exchange() const { return exchange_; }
    void Exchange(Proto::Exchange exchange) { exchange_ = exchange; }

    const Proto::InstrumentType Type() const { return type_; }
    void Type(Proto::InstrumentType type) { type_ = type; }

    const Proto::InstrumentStatus Status() const { return status_; }
    void Status(Proto::InstrumentStatus status) { status_ = status; }

    const Proto::Currency Currency() const { return currency_; }
    void Currency(Proto::Currency currency) { currency_ = currency; }

    const base::PriceType Tick() const { return tick_; }
    void Tick(base::PriceType tick) { tick_ = tick; }

    const base::PriceType Multiplier() const { return multiplier_; }
    void Multiplier(base::PriceType multiplier) { multiplier_ = multiplier; }

    const base::PriceType Highest() const { return highest_; }
    void Highest(base::PriceType highest) { highest_ = highest; }

    const base::PriceType Lowest() const { return lowest_; }
    void Lowest(base::PriceType lowest) { lowest_ = lowest; }

    const boost::gregorian::date& Maturity() const { return maturity_; }
    void Maturity(int year, int month, int day)
    {
      maturity_ = boost::gregorian::date(year, month, day);
    }
    void Maturity(const boost::gregorian::date& maturity) { maturity_ = maturity; }

    const Instrument* Underlying() const { return underlying_; }
    void Underlying(const Instrument* underlying) { underlying_ = underlying; }

    const Instrument* HedgeUnderlying() const { return hedge_underlying_; }
    void HedgeUnderlying(const Instrument* hedge_undl) { hedge_underlying_ = hedge_undl; }

    void Serialize(Proto::Instrument *inst) const;

    base::TickType ConvertToTick(base::PriceType p) const { return (p + 0.5 * tick_) / tick_; }

  protected:
    std::string id_;
    std::string symbol_;
    Proto::Exchange exchange_;
    Proto::InstrumentType type_;
    std::atomic<Proto::InstrumentStatus> status_;
    Proto::Currency currency_;
    base::PriceType tick_ = base::PRICE_UNDEFINED;
    base::PriceType multiplier_ = base::PRICE_UNDEFINED;
    base::PriceType highest_ = base::PRICE_UNDEFINED;
    base::PriceType lowest_ = base::PRICE_UNDEFINED;
    boost::gregorian::date maturity_;

    const Instrument* underlying_ = nullptr;
    const Instrument* hedge_underlying_ = nullptr;
};

#endif
