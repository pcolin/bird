#include "ProductManager.h"
#include "base/logger/Logging.h"

#include <utility>
// #include <boost/format.hpp>

using namespace base;

ProductManager* ProductManager::GetInstance()
{
  static ProductManager manager;
  return &manager;
}

ProductManager::~ProductManager()
{
  for (auto& it : instruments_)
  {
    delete it.second;
  }
  // instruments_.clear();
}

const Instrument* ProductManager::Add(Instrument* instrument)
{
  if (instrument)
  {
    std::lock_guard<std::mutex> lck(mtx_);
    auto it = instruments_.find(instrument->Id());
    if (it == instruments_.end())
    {
      LOG_INF << "Insert instrument " << instrument->Id();
      instruments_.insert(std::make_pair(instrument->Id(), instrument));
    }
    else
    {
      LOG_WAN << "Duplicated instrument " << instrument->Id();
      instrument = it->second;
    }
  }
  return instrument;
}

void ProductManager::Remove(const Instrument* instrument)
{
  std::lock_guard<std::mutex> lck(mtx_);
  auto it = instruments_.find(instrument->Id());
  if (it != instruments_.end())
  {
    instruments_.erase(it);
  }
}

const Instrument* ProductManager::FindId(const std::string& id)
{
  std::lock_guard<std::mutex> lck(mtx_);
  auto it = instruments_.find(id);
  return it != instruments_.end() ? it->second : nullptr;
}

const Instrument* ProductManager::FindSymbol(const std::string& symbol)
{
  std::lock_guard<std::mutex> lck(mtx_);
  for (const auto& it : instruments_)
  {
    if (it.first.find(symbol) != std::string::npos) return it.second;
  }
  return nullptr;
}

const std::vector<const Instrument*>
ProductManager::FindInstruments(std::function<bool(const Instrument*)> filter)
{
  std::vector<const Instrument*> instruments;
  std::lock_guard<std::mutex> lck(mtx_);
  for (const auto& it : instruments_)
  {
    if (filter(it.second)) instruments.push_back(it.second);
  }
  return instruments;
}