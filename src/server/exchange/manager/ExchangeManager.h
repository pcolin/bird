#ifndef EXCHANGE_MANAGER_H
#define EXCHANGE_MANAGER_H

#include <memory>
#include "TraderApi.h"
#include "MdApi.h"

class ExchangeManager
{
  public:
    static ExchangeManager* GetInstance();
    ~ExchangeManager();

    void Init();
    TraderApi* GetTraderApi() { return trader_; }

  private:
    ExchangeManager() {}

    TraderApi *trader_ = nullptr;
    MdApi *md_;
};

#endif
