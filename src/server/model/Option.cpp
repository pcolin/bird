#include "Option.h"
// #include "Message.h"

// std::shared_ptr<Proto::Instrument> Option::Serialize() const {
//   auto inst = Message::NewProto<Proto::Instrument>();
//   inst->set_id(id_);
//   inst->set_symbol(symbol_);
//   inst->set_exchange(exchange_);
//   inst->set_type(type_);
//   inst->set_currency(currency_);
//   inst->set_tick(tick_);
//   inst->set_multiplier(multiplier_);
//   if (underlying_) {
//     inst->set_underlying(underlying_->Id());
//   }
//   if (hedge_underlying_) {
//     inst->set_hedge_underlying(hedge_underlying_->Id());
//   }
//   inst->set_maturity(boost::gregorian::to_iso_string(maturity_));
//   inst->set_call_put(call_put_);
//   inst->set_exercise(exercise_type_);
//   inst->set_settlement(settlement_type_);
//   inst->set_strike(strike_);
//   return inst;
// }

void Option::Serialize(Proto::Instrument *inst) const {
  Instrument::Serialize(inst);
  inst->set_call_put(call_put_);
  inst->set_exercise(exercise_type_);
  inst->set_settlement(settlement_type_);
  inst->set_strike(strike_);
}
