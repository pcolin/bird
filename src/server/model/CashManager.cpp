#include "CashManager.h"

CashManager* CashManager::GetInstance()
{
  static CashManager manager;
  return &manager;
}

void CashManager::OnCash(const std::shared_ptr<proto::Cash> &cash)
{

}
