#include "SimulationMdApi.h"
#include "base/common/Likely.h"
#include "base/logger/Logging.h"
#include "model/ProductManager.h"
#include "strategy/ClusterManager.h"

#include <ctime>
#include <cstdlib>
#include <iostream>

void SimulationMdApi::Init()
{
  std::srand(std::time(nullptr));
  auto insts = ProductManager::GetInstance()->FindInstruments([](const Instrument *inst)
      { return inst->Type() == Proto::InstrumentType::Option; });
  for (auto *inst : insts)
  {
    auto p = Message::NewPrice();
    p->instrument = inst;
    auto *underlying = inst->Underlying();
    auto it = prices_.find(underlying->Id());
    if (it == prices_.end())
    {
      auto up = Message::NewPrice();
      up->instrument = underlying;
      up->pre_close = inst->RoundToTick(3100 - std::rand() % 100, Proto::RoundDirection::Nearest);
      up->pre_settlement = up->pre_close - std::rand() % 50;
      Instrument *tmp = const_cast<Instrument*>(underlying);
      tmp->Lowest(underlying->RoundToTick(up->pre_settlement * 0.95,
            Proto::RoundDirection::Nearest));
      tmp->Highest(underlying->RoundToTick(up->pre_settlement * 1.05,
            Proto::RoundDirection::Nearest));
      up->adjust = (inst->Maturity() - boost::gregorian::day_clock::local_day()).days() / 245;
      it = prices_.emplace(underlying->Id(), up).first;
    }
    auto *option = dynamic_cast<const Option*>(inst);
    if (option->CallPut() == Proto::OptionType::Call)
    {
      p->pre_close = std::max(it->second->pre_close - option->Strike() + it->second->adjust,
          inst->Tick());
    }
    else
    {
      p->pre_close = std::max(option->Strike() - it->second->pre_close + it->second->adjust,
          inst->Tick());
    }
    p->pre_settlement = std::max(p->pre_close - std::rand() % 20, inst->Tick());
    Instrument *tmp = const_cast<Instrument*>(inst);
    tmp->Lowest(inst->RoundToTick(p->pre_settlement * 0.95, Proto::RoundDirection::Nearest));
    tmp->Highest(inst->RoundToTick(p->pre_settlement * 1.05, Proto::RoundDirection::Nearest));
    prices_.emplace(inst->Id(), p);
  }
  td_ = std::make_unique<std::thread>(std::bind(&SimulationMdApi::Work, this));
}

void SimulationMdApi::Work()
{
  LOG_INF << "Simulation MD is running...";
  std::srand(std::time(nullptr));
  while (true)
  {
    double chg = 0;
    // std::cout << "Simulated MD is running..." << std::endl;
    for (auto &it : prices_)
    {
      if (std::rand() % 4 != 0) continue;

      auto *inst = it.second->instrument;
      auto *dm = ClusterManager::GetInstance()->FindDevice(inst->HedgeUnderlying());
      if (unlikely(dm == nullptr))
      {
        LOG_ERR << "Can't find device manger for " << inst->HedgeUnderlying();
        continue;
      }

      if (inst->Type() != Proto::InstrumentType::Option)
      {
        chg = (1 - std::rand() % 3) * 0.001;
        it.second->adjust = PRICE_UNDEFINED;
      }

      auto p = Message::NewPrice();
      *p = *it.second;
      p->header.SetTime();

      if (p->last.price != PRICE_UNDEFINED)
      {
        if (inst->Type() == Proto::InstrumentType::Option &&
            dynamic_cast<const Option*>(inst)->CallPut() == Proto::OptionType::Put)
        {
          p->last.price -= inst->RoundToTick(chg * p->last.price, Proto::RoundDirection::Nearest);
        }
        else
        {
          p->last.price += inst->RoundToTick(chg * p->last.price, Proto::RoundDirection::Nearest);
        }
        p->last.price = std::max(inst->Tick(), p->last.price);
        p->last.volume += std::max(1, 50 - std::rand() % 100);
        p->high = std::max(p->high, p->last.price);
        p->low = std::min(p->low, p->last.price);
      }
      else
      {
        if (inst->Type() == Proto::InstrumentType::Option &&
            dynamic_cast<const Option*>(inst)->CallPut() == Proto::OptionType::Put)
        {
          p->open = inst->RoundToTick(p->pre_settlement * (1 - chg), Proto::RoundDirection::Nearest);
        }
        else
        {
          p->open = inst->RoundToTick(p->pre_settlement * (1 + chg), Proto::RoundDirection::Nearest);
        }
        p->open = std::max(inst->Tick(), p->open);
        it.second->open = p->open;
        it.second->high = p->open;
        it.second->low = p->open;
        p->last.price = p->open;
        p->last.volume = std::max(1, 50 - std::rand() % 100);
        p->high = p->open;
        p->low = p->open;
      }
      it.second->last = p->last;

      for (int i = 0; i < Price::LEVELS; ++i)
      {
        p->bids[i].price = p->last.price - (i + 1) * inst->Tick();
        if (p->bids[i].price < inst->Tick()) break;
        p->bids[i].volume = std::rand() % 100 + 1;
      }

      for (int i = 0; i < Price::LEVELS; ++i)
      {
        p->asks[i].price = p->last.price + (i + 1) * inst->Tick();
        p->asks[i].volume = std::rand() % 100 + 1;
      }
      // std::cout << "Publish " << it.first << " : " << p->last.price << std::endl;
      dm->Publish(p);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
  }
  LOG_INF << "Simulation MD is stopped";
}
