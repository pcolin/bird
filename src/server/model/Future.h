#ifndef MODEL_FUTURE_H
#define MODEL_FUTURE_H

#include "Instrument.h"

#include <boost/date_time/gregorian/gregorian.hpp>

class Future : public Instrument
{
  public:
    Future() : Instrument(InstrumentType::Future) {}

    const boost::gregorian::date& Maturity() const { return maturity_; }
    void Maturity(int year, int month, int day)
    {
      maturity_ = boost::gregorian::date(year, month, day);
    }
    void Maturity(const boost::gregorian::date& maturity) { maturity_ = maturity; }

  private:
    boost::gregorian::date maturity_;
};

#endif
