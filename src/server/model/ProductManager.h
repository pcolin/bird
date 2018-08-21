#ifndef MODEL_PRODUCT_MANAGER_H
#define MODEL_PRODUCT_MANAGER_H

#include <mutex>
#include <vector>
#include <unordered_map>
#include <functional>
#include "Option.h"

class ProductManager
{
  public:
    static ProductManager* GetInstance();
    ~ProductManager();

    const Instrument* Add(Instrument* instrument);
    void Remove(const Instrument* instrument);
    const Instrument* FindId(const std::string& id);
    const Instrument* FindSymbol(const std::string& symbol);
    const std::vector<const Option*> FindOptions(const Instrument *hedge_underlying);
    const std::vector<const Instrument*>
      FindInstruments(const std::function<bool(const Instrument*)> &filter );

  private:
    ProductManager() {}
    std::unordered_map<std::string, Instrument*> instruments_;

    std::mutex mtx_;
};

#endif
