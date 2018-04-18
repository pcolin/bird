#ifndef DATABASED_DB_BASE_H
#define DATABASED_DB_BASE_H

#include "base/common/ProtoMessageDispatcher.h"
#include "sqlite3.h"

#include <mutex>

class ConcurrentSqliteDB
{
public:
  ConcurrentSqliteDB(const std::string &file);
  ~ConcurrentSqliteDB();

  bool ExecSql(const char *sql, void *data, sqlite3_callback cb);
  void BeginTransaction()
  {
    sqlite3_exec(db_, "begin;", 0, 0, 0);
  }
  void CommitTransaction()
  {
    sqlite3_exec(db_, "commit;", 0, 0, 0);
  }

private:
  bool Open(const std::string &file);
  bool Close();

  sqlite3 *db_ = nullptr;
  std::mutex mtx_;
};

class DbBase
{
public:
  DbBase(ConcurrentSqliteDB &db, const std::string &table_name);
  virtual ~DbBase() {}

  virtual bool Initialize(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher);

  const std::string& TableName() const { return table_name_; }

protected:
  bool ExecSql(const char *sql, void *data = nullptr, sqlite3_callback cb = Callback)
  {
    return db_.ExecSql(sql, data, cb);
  }

  class TransactionGuard
  {
  public:
    TransactionGuard(DbBase *db) : db_(db)
    {
      db_->db_.BeginTransaction();
    }
    ~TransactionGuard()
    {
      db_->db_.CommitTransaction();
    }
  private:
    DbBase *db_;
  };

  std::string table_name_;

private:
  virtual void RefreshCache() = 0;
  virtual void RegisterCallback(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher) = 0;
  static int Callback(void *data, int argc, char **argv, char **col_name) {}

  ConcurrentSqliteDB &db_;
};

#endif
