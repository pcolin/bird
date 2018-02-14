#ifndef DATABASED_DB_BASE_H_
#define DATABASED_DB_BASE_H_

#include "base/common/ProtoMessageDispatcher.h"
#include "sqlite3.h"

#include <mutex>

class ConcurrentSqliteDB
{
public:
  ConcurrentSqliteDB(const std::string &file);
  ~ConcurrentSqliteDB();

  bool ExecSql(const char *sql, void *data, sqlite3_callback cb);

private:
  bool Open(const std::string &file);
  bool Close();

  sqlite3 *db_ = nullptr;
  std::mutex mtx_;
};

class DbBase
{
public:
  DbBase(ConcurrentSqliteDB &db);
  virtual ~DbBase() {}

  virtual bool Initialize(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

protected:
  bool ExecSql(const char *sql, void *data = nullptr, sqlite3_callback cb = Callback)
  {
    return db_.ExecSql(sql, data, cb);
  }

private:
  virtual void RefreshCache() = 0;
  virtual void RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher) = 0;
  static int Callback(void *data, int argc, char **argv, char **col_name) {}

  ConcurrentSqliteDB &db_;
};

#endif
