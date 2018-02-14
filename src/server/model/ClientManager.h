#ifndef MODEL_CLIENT_MANAGER_H
#define MODEL_CLIENT_MANAGER_H

#include "Login.pb.h"
#include "Reply.pb.h"
#include "Heartbeat.pb.h"
#include <unordered_map>
#include <mutex>

class ClientManager
{
  // typedef std::shared_ptr<google::protobuf::Message> ProtoMessagePtr;
  typedef std::unordered_map<std::string, std::tuple<Proto::Role, int64_t>> ClientHeartbeatMap;
public:
  static ClientManager* GetInstance();
  ~ClientManager() {}

  std::shared_ptr<Proto::Reply> Login(const std::shared_ptr<Proto::Login> &login);
  std::shared_ptr<Proto::Reply> Logout(const std::shared_ptr<Proto::Logout> &logout);

  void OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat);

private:
  ClientManager() {}

  ClientHeartbeatMap clients_;
  std::mutex mtx_;
};

#endif
