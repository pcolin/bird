#include "DbBase.h"
#include "base/common/Likely.h"
#include "base/logger/Logging.h"
#include "boost/format.hpp"

using namespace base;

ConcurrentSqliteDB::ConcurrentSqliteDB(const std::string &file) { Open(file); }

ConcurrentSqliteDB::~ConcurrentSqliteDB() { Close(); }

bool ConcurrentSqliteDB::ExecSql(const char *sql, void *data, sqlite3_callback cb) {
  LOG_INF << sql;
  int rc;
  char *err;
  {
    std::lock_guard<std::mutex> lck(mtx_);
    rc = sqlite3_exec(db_, sql, cb, data, &err);
  }
  if (unlikely(rc != SQLITE_OK)) {
    LOG_ERR << boost::format("Failed to exec Sql(%1%): %2%") % sql % err;
    return false;
  }
  return true;
}

bool ConcurrentSqliteDB::Open(const std::string &file) {
  if (db_ == nullptr) {
    if (sqlite3_open(file.c_str(), &db_)) {
      LOG_ERR << "Can't open database: " << sqlite3_errmsg(db_);
      sqlite3_close(db_);
      return false;
    } else {
      sqlite3_exec(db_, "PRAGMA synchronous = OFF;", 0, 0, 0);
    }
  }
  return true;
}

bool ConcurrentSqliteDB::Close() {
  if (db_) {
    sqlite3_close(db_);
    db_ = nullptr;
  }
  return true;
}

DbBase::DbBase(ConcurrentSqliteDB &db, const std::string &table_name)
    : db_(db), table_name_(table_name) {}

bool DbBase::Initialize(base::ProtoMessageDispatcher<base::ProtoMessagePtr> &dispatcher) {
  RefreshCache();
  RegisterCallback(dispatcher);
  return true;
}
