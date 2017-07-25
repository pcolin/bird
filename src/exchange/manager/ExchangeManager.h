#ifndef EXCHANGE_MANAGER_H
#define EXCHANGE_MANAGER_H

#include <memory>
#include "TraderApi.h"
#include "MdApi.h"

class ExchangeManager
{
  public:
    static ExchangeManager* GetInstance();

    void Init();

  private:
    ExchangeManager() {}

    std::shared_ptr<TraderApi> trader_;
    std::shared_ptr<MdApi> md_;
};

#endif
