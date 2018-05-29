#include "Instrument.h"

#include "base/common/Itoa.h"

void Instrument::Serialize(Proto::Instrument *inst) const
{
  inst->set_id(id_);
  inst->set_symbol(symbol_);
  inst->set_exchange(exchange_);
  inst->set_type(type_);
  inst->set_currency(currency_);
  inst->set_tick(tick_);
  inst->set_multiplier(multiplier_);
  if (underlying_)
  {
    inst->set_underlying(underlying_->Id());
  }
  if (hedge_underlying_)
  {
    inst->set_hedge_underlying(hedge_underlying_->Id());
  }

  char tmp[9];
  base::convert(tmp, (int)maturity_.year());
  int month = maturity_.month();
  if (month > 9)
  {
    base::convert(tmp + 4, month);
  }
  else
  {
    tmp[4] = '0';
    base::convert(tmp + 5, month);
  }
  int day = maturity_.day();
  if (day > 9)
  {
    base::convert(tmp + 6, day);
  }
  else
  {
    tmp[6] = '0';
    base::convert(tmp + 7, day);
  }
  inst->set_maturity(tmp);
  inst->set_status(status_);
}
