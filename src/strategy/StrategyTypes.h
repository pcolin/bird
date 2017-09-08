#ifndef STRATEGY_STRATEGY_TYPES_H
#define STRATEGY_STRATEGY_TYPES_H

#include "model/Price.h"
#include "model/Order.h"
#include "model/Trade.h"
#include "base/disruptor/RingBuffer.h"
#include <boost/variant.hpp>

const int BUFFER_SIZE = 4 * 1024;

typedef boost::variant<std::shared_ptr<Price>,
                       std::shared_ptr<Order>,
                       std::shared_ptr<Trade>> Event;
typedef base::RingBuffer<Event, BUFFER_SIZE> StrategyRingBuffer;
// typedef BatchEventProcessor<StrategyRingBuffer> StrategyEventProcessor;

#endif
