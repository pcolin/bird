#ifndef MODEL_POSITION_MANAGER_H
#define MODEL_POSITION_MANAGER_H

#include "Trade.h"
#include "Position.pb.h"

#include <unordered_map>
#include <mutex>

// struct Position
// {
//   base::VolumeType total_long;
//   base::VolumeType liquid_long;
//   base::VolumeType yesterday_long;

//   base::VolumeType total_short;
//   base::VolumeType liquid_short;
//   base::VolumeType yesterday_short;
// };

// typedef std::shared_ptr<Position> PositionPtr;
typedef std::shared_ptr<Proto::Position> PositionPtr;

class PositionManager
{
public:
  static PositionManager* GetInstance();
  ~PositionManager() {}

  void Init();
 
  bool TryFreeze(const OrderPtr &order);
  void Release(const OrderPtr &order);

  void UpdatePosition(const PositionPtr &position);
  void OnTrade(const TradePtr &trade);

private:
  PositionManager() {}

  void PublishPosition(Proto::Exchange exchange, PositionPtr &position);

  typedef std::unordered_map<const Instrument*, PositionPtr> PositionMap;
  PositionMap positions_;
  std::mutex mtx_;

};

#endif
