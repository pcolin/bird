#ifndef MODEL_CLIENT_MANAGER_H
#define MODEL_CLIENT_MANAGER_H

#include <unordered_map>
#include <mutex>
#include "Login.pb.h"
#include "Reply.pb.h"
#include "Heartbeat.pb.h"

class ClientManager {
 public:
  typedef std::shared_ptr<Proto::Reply> ProtoReplyPtr;
  typedef std::unordered_map<std::string, std::tuple<Proto::Role, int64_t>> ClientHeartbeatMap;

  static ClientManager* GetInstance();
  ~ClientManager() {}

  ProtoReplyPtr Login(const std::shared_ptr<Proto::Login> &login);
  ProtoReplyPtr Logout(const std::shared_ptr<Proto::Logout> &logout);

  ProtoReplyPtr OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat);

 private:
  ClientManager() {}

  ClientHeartbeatMap clients_;
  std::mutex mtx_;
};

#endif // MODEL_CLIENT_MANAGER_H
