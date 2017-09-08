#include "Order.h"

#include <boost/uuid/uuid_generators.hpp>

thread_local boost::uuids::random_generator id_generator;

Order::Order()
  : header(MsgType::Order),
  local_id(boost::uuids::hash_value(id_generator()))
{
  header.SetTime();
}
