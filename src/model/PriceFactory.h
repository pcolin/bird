#ifndef MODEL_PRICE_FACTORY_H
#define MODEL_PRICE_FACTORY_H

#include "Price.h"
#include "base/memory/MemoryPool.h"

#include <mutex>
#include <memory>

class PriceFactory
{
  public:
    static PriceFactory* GetInstance();
    // Price* Allocate();
    std::shared_ptr<Price> Allocate();
    // void Deallocate(Price *price);

  private:
    PriceFactory() {}
    MemoryPool<Price, 1000*sizeof(Price)> pool_;
    std::mutex mtx_;
};

#endif
