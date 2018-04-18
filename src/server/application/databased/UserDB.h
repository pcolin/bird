#ifndef DATABASED_USER_DB_H
#define DATABASED_USER_DB_H

#include "DbBase.h"
#include "Login.pb.h"

#include <unordered_map>
#include <tuple>

class UserDB : public DbBase
{
  typedef std::unordered_map<std::string, std::tuple<std::string, Proto::Role>> UserMap;
public:
  UserDB(ConcurrentSqliteDB &db, const std::string &table_name);
  virtual ~UserDB() {}

private:
  virtual void RefreshCache() override;
  virtual void RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  base::ProtoMessagePtr OnLogin(const std::shared_ptr<Proto::Login> &msg);

  static int Callback(void *data, int argc, char **argv, char **col_name);

  UserMap cache_;
};

#endif
