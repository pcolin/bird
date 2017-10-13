#ifndef STRATEGY_DEVICE_MANAGER_H
#define STRATEGY_DEVICE_MANAGER_H

#include "StrategyTypes.h"
#include "Strategy.pb.h"
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
  const Instrument* GetUnderlying() const
  {
    return underlying_;
  }

  template<class E> void Publish(E &e)
  {
    int64_t seq = rb_.Next();
    rb_.Get(seq) = std::move(e);
    rb_.Publish(seq);
  }

  void Start(const std::string& name);
  void StartAll();
  void Stop(const std::string& name);
  void StopAll();
  // void Publish(std::shared_ptr<Price> &price);
  std::shared_ptr<StrategyDevice> FindStrategyDevice(const std::string &name) const;

  void OnStrategyStatusReq(const std::shared_ptr<proto::StrategyStatusReq> &msg);

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
