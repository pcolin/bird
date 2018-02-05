#include "Strategy.h"
#include "model/Middleware.h"
// #include <sys/time.h>

Strategy::Strategy(const std::string &name, DeviceManager *dm)
  : name_(name), dm_(dm), visitor_(this)
{
  dispatcher_.RegisterCallback<proto::Heartbeat>(
      std::bind(&Strategy::OnHeartbeat, this, std::placeholders::_1));
}

bool Strategy::OnHeartbeat(const std::shared_ptr<proto::Heartbeat> &heartbeat)
{
  auto h = Message::NewProto<proto::Heartbeat>();
  h->set_name(name_);
  // google::protobuf::Timestamp *t = new google::protobuf::Timestamp;
  // t->set_seconds(time(NULL));
  // struct timeval tv;
  // gettimeofday(&tv, NULL);
  // t->set_seconds(tv.tv_sec);
  // t->set_nanos(tv.tv_usec * 1000);
  // h->set_allocated_time(t);
  h->set_time(time(NULL));
  Middleware::GetInstance()->Publish(h);
  return true;
}