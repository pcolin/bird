#ifndef MODEL_FUTURE_H
#define MODEL_FUTURE_H

#include "Instrument.h"

class Future : public Instrument {
 public:
  Future() : Instrument(Proto::InstrumentType::Future) {}

  // std::shared_ptr<Proto::Instrument> Serialize() const;
  // void Serialize(Proto::Instrument *inst) const;
};

#endif // MODEL_FUTURE_H
