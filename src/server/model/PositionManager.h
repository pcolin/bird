#ifndef MODEL_POSITION_MANAGER_H
#define MODEL_POSITION_MANAGER_H

#include <unordered_map>
#include <mutex>
#include "Trade.h"
#include "Position.pb.h"

typedef std::shared_ptr<Proto::Position> PositionPtr;

class PositionManager {
 public:
  static PositionManager* GetInstance();
  ~PositionManager() {}

  void Init();

  int GetNetPosition(const Instrument *inst);

  bool TryFreeze(const OrderPtr &order);
  void Release(const OrderPtr &order);

  void UpdatePosition(const PositionPtr &position);
  void OnTrade(const TradePtr &trade);

 private:
  PositionManager() {}

  void PublishPosition(PositionPtr &position);

  typedef std::unordered_map<const Instrument*, PositionPtr> PositionMap;
  PositionMap positions_;
  std::mutex mtx_;
};

#endif // MODEL_POSITION_MANAGER_H
