#include "UserDB.h"
#include "base/logger/Logging.h"
#include "model/Message.h"
#include "Reply.pb.h"

UserDB::UserDB(ConcurrentSqliteDB &db, const std::string &table_name)
  : DbBase(db), table_name_(table_name)
{}

void UserDB::RefreshCache()
{
  char query[1024];
  sprintf(query, "SELECT * FROM %s", table_name_.c_str());
  ExecSql(query, &cache_, &UserDB::Callback);
}

void UserDB::RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher)
{
  dispatcher.RegisterCallback<Proto::Login>(
    std::bind(&UserDB::OnLogin, this, std::placeholders::_1));
  // dispatcher.RegisterCallback<Proto::Logout>(
  //   std::bind(&UserDB::OnLogout, this, std::placeholders::_1));
}

base::ProtoMessagePtr UserDB::OnLogin(const std::shared_ptr<Proto::Login> &msg)
{
  LOG_INF << "Login request: " << msg->ShortDebugString();
  auto reply = Message::NewProto<Proto::Reply>();
  auto it = cache_.find(msg->user());
  if (it != cache_.end())
  {
    if (msg->password() != std::get<0>(it->second))
    {
      reply->set_result(false);
      reply->set_error("wrong password");
    }
    else if (msg->role() != std::get<1>(it->second))
    {
      reply->set_result(false);
      reply->set_error("wrong role");
    }
    else
    {
      reply->set_result(true);
    }
  }
  else
  {
    reply->set_result(false);
    reply->set_error("unknown user");
  }
  return reply;
}

int UserDB::Callback(void *data, int argc, char **argv, char **col_name)
{
  auto &cache = *static_cast<UserMap*>(data);
  cache[argv[0]] = std::make_tuple(argv[1], static_cast<Proto::Role>(std::stoi(argv[2])));
  return 0;
}
