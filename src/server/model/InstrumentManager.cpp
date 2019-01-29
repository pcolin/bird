#include <utility>
#include "InstrumentManager.h"
#include "base/logger/Logging.h"

using namespace base;

InstrumentManager* InstrumentManager::GetInstance() {
  static InstrumentManager manager;
  return &manager;
}

InstrumentManager::~InstrumentManager() {
  for (auto& it : instruments_) {
    delete it.second;
  }
  // instruments_.clear();
}

const Instrument* InstrumentManager::Add(Instrument* instrument) {
  if (instrument) {
    std::lock_guard<std::mutex> lck(mtx_);
    auto it = instruments_.find(instrument->Id());
    if (it == instruments_.end()) {
      LOG_INF << "Insert instrument " << instrument->Id();
      instruments_.insert(std::make_pair(instrument->Id(), instrument));
    } else {
      LOG_WAN << "Duplicated instrument " << instrument->Id();
      instrument = it->second;
    }
  }
  return instrument;
}

void InstrumentManager::Remove(const Instrument* instrument) {
  std::lock_guard<std::mutex> lck(mtx_);
  auto it = instruments_.find(instrument->Id());
  if (it != instruments_.end()) {
    instruments_.erase(it);
  }
}

const Instrument* InstrumentManager::FindId(const std::string& id) {
  std::lock_guard<std::mutex> lck(mtx_);
  auto it = instruments_.find(id);
  return it != instruments_.end() ? it->second : nullptr;
}

const Instrument* InstrumentManager::FindSymbol(const std::string& symbol) {
  std::lock_guard<std::mutex> lck(mtx_);
  for (const auto& it : instruments_) {
    if (it.first.find(symbol) != std::string::npos) return it.second;
  }
  return nullptr;
}

const std::vector<const Option*> InstrumentManager::FindOptions(const Instrument *hedge_underlying) {
  std::vector<const Option*> options;
  std::lock_guard<std::mutex> lck(mtx_);
  for (const auto& it : instruments_) {
    if (it.second->Type() == Proto::InstrumentType::Option &&
        it.second->HedgeUnderlying() == hedge_underlying) {
      auto *op = base::down_cast<const Option*>((const Instrument*)it.second);
      options.push_back(op);
    }
  }
  return options;
}

const Instrument* InstrumentManager::FindInstrument(
    const std::function<bool(const Instrument*)> &filter) {
  std::lock_guard<std::mutex> lck(mtx_);
  for (const auto& it : instruments_) {
    if (filter && filter(it.second)) return it.second;
  }
  return nullptr;
}

const std::vector<const Instrument*> InstrumentManager::FindInstruments(
    const std::function<bool(const Instrument*)> &filter) {
  std::vector<const Instrument*> instruments;
  std::lock_guard<std::mutex> lck(mtx_);
  for (const auto& it : instruments_) {
    if (filter && filter(it.second)) instruments.push_back(it.second);
  }
  return instruments;
}
