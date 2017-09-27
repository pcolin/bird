#ifndef MODEL_CASH_MANAGER_H
#define MODEL_CASH_MANAGER_H

class CashManager
{
public:
  static CashManager* GetInstance();
  ~CashManager();

private:
  CashManager() {}
};

#endif
