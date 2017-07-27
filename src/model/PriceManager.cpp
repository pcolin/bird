#include "PriceManager.h"
// #include <iostream>

PriceManager* PriceManager::GetInstance()
{
  static PriceManager manager;
  return &manager;
}

std::shared_ptr<Price> PriceManager::Allocate()
{
  Price *price = nullptr;
  {
    std::lock_guard<std::mutex> lck(mtx_);
    price = pool_.allocate();
    // std::cout << "New from pool " << (void*)price << std::endl;
  }
  assert(price);
  return std::shared_ptr<Price>(price, [this](Price *p)
      {
        std::lock_guard<std::mutex> lck(mtx_);
        pool_.deallocate(p);
        // std::cout << "Delete from lamda " << (void*)p << std::endl;
      });
}
