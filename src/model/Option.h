#ifndef MODEL_OPTION_H
#define MODEL_OPTION_H

#include "Instrument.h"
#include "Future.h"
#include <boost/date_time/gregorian/gregorian.hpp>

namespace base
{
  enum OptionType
  {
    Call = 0,
    Put = 1,
  };

  enum ExerciseType
  {
    European = 0,
    American = 1,
  };

  enum SettlementType
  {
    CashSettlement = 0,
    PhysicalSettlement = 1,
  };
}

class Option : public Instrument
{
  public:
    Option() : Instrument(InstrumentType::Option) {}

    base::OptionType CallPut() const { return call_put_; }
    void CallPut(base::OptionType call_put) { call_put_ = call_put; }

    base::ExerciseType ExerciseType() const { return exercise_type_; }
    void ExerciseType(base::ExerciseType exercise_type) { exercise_type_ = exercise_type; }

    base::SettlementType SettlementType() const { return settlement_type_; }
    void SettlementType(base::SettlementType settlement_type) { settlement_type_ = settlement_type; }

    base::PriceType Strike() const { return strike_; }
    void Strike(base::PriceType strike) { strike_ = strike; }

    const boost::gregorian::date& Maturity() const { return maturity_; }
    void Maturity(int year, int month, int day)
    {
      maturity_ = boost::gregorian::date(year, month, day);
    }
    void Maturity(const boost::gregorian::date& maturity) { maturity_ = maturity; }

  private:
    base::OptionType call_put_;
    base::ExerciseType exercise_type_;
    base::SettlementType settlement_type_;
    base::PriceType strike_;
    boost::gregorian::date maturity_;
};

#endif
