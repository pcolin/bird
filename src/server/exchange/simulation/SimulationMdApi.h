#ifndef SIMULATION_MD_API_H
#define SIMULATION_MD_API_H

#include "../manager/MdApi.h"
#include "model/Price.h"
#include <map>
#include <thread>

class SimulationMdApi : public MdApi
{
public:
  void Init() override;

private:
  void Work();

  std::unique_ptr<std::thread> td_;
  std::map<std::string, PricePtr> prices_;
};

#endif
