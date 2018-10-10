#ifndef MODEL_OPTION_H
#define MODEL_OPTION_H

#include "Instrument.h"

class Option : public Instrument {
 public:
  Option() : Instrument(Proto::InstrumentType::Option) {}

  Proto::OptionType CallPut() const { return call_put_; }
  void CallPut(Proto::OptionType call_put) { call_put_ = call_put; }

  Proto::ExerciseType ExerciseType() const { return exercise_type_; }
  void ExerciseType(Proto::ExerciseType exercise_type) { exercise_type_ = exercise_type; }

  Proto::SettlementType SettlementType() const { return settlement_type_; }
  void SettlementType(Proto::SettlementType settlement_type) { settlement_type_ = settlement_type; }

  base::PriceType Strike() const { return strike_; }
  void Strike(base::PriceType strike) { strike_ = strike; }

  // std::shared_ptr<Proto::Instrument> Serialize() const;
  void Serialize(Proto::Instrument *inst) const;

 private:
  Proto::OptionType call_put_;
  Proto::ExerciseType exercise_type_;
  Proto::SettlementType settlement_type_;
  base::PriceType strike_;
};

#endif // MODEL_OPTION_H
