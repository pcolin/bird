#ifndef MODEL_POSITION_MANAGER_H
#define MODEL_POSITION_MANAGER_H

#include "Order.h"

class PositionManager
{
public:
  static PositionManager* GetInstance();
  ~PositionManager() {}

  void Init();
 
  bool TryFreeze(const OrderPtr &order);
  void Release(const OrderPtr &order);

private:
  PositionManager() {}

};

#endif
