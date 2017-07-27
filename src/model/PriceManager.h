#ifndef MODEL_PRICE_MANAGER_H
#define MODEL_PRICE_MANAGER_H

#include "Message.h"
#include "base/memory/MemoryPool.h"

#include <mutex>
#include <memory>

class PriceManager
{
  public:
    static PriceManager* GetInstance();
    // Price* Allocate();
    std::shared_ptr<Price> Allocate();
    // void Deallocate(Price *price);

  private:
    PriceManager() {}
    MemoryPool<Price, 1000*sizeof(Price)> pool_;
    std::mutex mtx_;
};

#endif
