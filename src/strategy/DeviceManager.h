#ifndef STRATEGY_DEVICE_MANAGER_H
#define STRATEGY_DEVICE_MANAGER_H

#include "StrategyTypes.h"
#include "base/disruptor/BusySpinWaitStrategy.h"
#include "base/disruptor/MultiProducerSequencer.h"

#include <unordered_map>

class StrategyDevice;
class DeviceManager
{
public:
  DeviceManager(const Instrument *underlying);
  DeviceManager(const DeviceManager&) = delete;
  DeviceManager& operator=(const DeviceManager&) = delete;
  ~DeviceManager();

  void Init();

  void Publish(std::shared_ptr<Price> &price);

private:
  const Instrument *underlying_;

  std::unordered_map<std::string, std::shared_ptr<StrategyDevice>> devices_;
  std::unique_ptr<StrategyDevice> monitor_;

  base::BusySpinWaitStrategy strategy_;
  base::MultiProducerSequencer<BUFFER_SIZE> sequencer_;
  StrategyRingBuffer rb_;
  std::vector<base::Sequence*> sequences_;
  base::SequenceBarrier *barrier_;
};

#endif
