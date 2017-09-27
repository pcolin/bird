#ifndef BASE_SYNC_LOGGING_H
#define BASE_SYNC_LOGGING_H

#include <mutex>
#include <boost/noncopyable.hpp>
#include "LogFile.h"

namespace base
{
class SyncLogging : boost::noncopyable
{
 public:
  SyncLogging(const std::string& path, const std::string& name)
    : logFile_(path + name)
  {}
  ~SyncLogging() {}

  void append(const char* logline, int len)
  {
    std::unique_lock<std::mutex> lock(mutex_);
    logFile_.append(logline, len);
    logFile_.flush();
  }

 private:
  LogFile logFile_;
  std::mutex mutex_;
};
}

#endif
