#ifndef STRATEGY_STRATEGY_TYPES_H
#define STRATEGY_STRATEGY_TYPES_H

#include "model/Price.h"
#include "model/Trade.h"
#include "base/disruptor/RingBuffer.h"
#include <boost/variant.hpp>
#include <google/protobuf/message.h>

const int BUFFER_SIZE = 4 * 1024;

typedef std::shared_ptr<google::protobuf::Message> ProtoMessagePtr;
typedef boost::variant<PricePtr, OrderPtr, TradePtr, ProtoMessagePtr> Event;
typedef base::RingBuffer<Event, BUFFER_SIZE> StrategyRingBuffer;
// typedef BatchEventProcessor<StrategyRingBuffer> StrategyEventProcessor;

#endif