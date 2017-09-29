#ifndef MODEL_CASH_MANAGER_H
#define MODEL_CASH_MANAGER_H

#include "Cash.pb.h"
#include <memory>

class CashManager
{
public:
  static CashManager* GetInstance();
  ~CashManager() {}

  void OnCash(const std::shared_ptr<PROTO::Cash> &cash);

private:
  CashManager() {}
};

#endif
