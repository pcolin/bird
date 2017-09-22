#include "PositionManager.h"

PositionManager* PositionManager::GetInstance()
{
  static PositionManager manager;
  return &manager;
}

void PositionManager::Init()
{
  /// to be done...
}

bool PositionManager::TryFreeze(const OrderPtr &order)
{
  /// to be done...
  return false;
}

void PositionManager::Release(const OrderPtr &order)
{
  /// to be done...
}
