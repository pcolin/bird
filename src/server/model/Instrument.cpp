#include "Instrument.h"
#include "base/common/Time.h"

void Instrument::Serialize(Proto::Instrument *inst) const {
  inst->set_id(id_);
  inst->set_symbol(symbol_);
  inst->set_product(product_);
  inst->set_exchange(exchange_);
  inst->set_type(type_);
  inst->set_currency(currency_);
  inst->set_lot(lot_);
  inst->set_tick(tick_);
  inst->set_multiplier(multiplier_);
  if (underlying_) {
    inst->set_underlying(underlying_->Id());
  }
  if (hedge_underlying_) {
    inst->set_hedge_underlying(hedge_underlying_->Id());
  }
  inst->set_highest(highest_);
  inst->set_lowest(lowest_);
  inst->set_commission(commission_type_);
  inst->set_open_commission(open_commission_);
  inst->set_close_commission(close_commission_);
  inst->set_close_today_commission(close_today_commission_);
  inst->set_maturity(base::DateToString(maturity_));
  inst->set_status(status_);
}
