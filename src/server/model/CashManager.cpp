#include "CashManager.h"

CashManager* CashManager::GetInstance()
{
  static CashManager manager;
  return &manager;
}
