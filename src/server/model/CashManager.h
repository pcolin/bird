#ifndef MODEL_CASH_MANAGER_H
#define MODEL_CASH_MANAGER_H

#include <memory>
#include "Cash.pb.h"

class CashManager {
 public:
  static CashManager* GetInstance();
  ~CashManager() {}

  void OnCash(const std::shared_ptr<Proto::Cash> &cash);

 private:
  CashManager() {}
};

#endif // MODEL_CASH_MANAGER_H
