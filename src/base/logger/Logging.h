#ifndef BASE_LOGGING_H
#define BASE_LOGGING_H

#include <functional>
#include "LogStream.h"
#include "Timestamp.h"

namespace base
{

class Logger
{
public:
  enum LogLevel
  {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    NUM_LOG_LEVELS,
  };

  // compile time calculation of basename of source file
  class SourceFile
  {
  public:
    template<int N>
    inline SourceFile(const char (&arr)[N])
      : data_(arr),
        size_(N-1)
    {
      const char* slash = strrchr(data_, '/'); // builtin function
      if (slash)
      {
        data_ = slash + 1;
        size_ -= static_cast<int>(data_ - arr);
      }
    }

    explicit SourceFile(const char* filename)
      : data_(filename)
    {
      const char* slash = strrchr(filename, '/');
      if (slash)
      {
        data_ = slash + 1;
      }
      size_ = static_cast<int>(strlen(data_));
    }
    const char* data_;
    int size_;
  };

  Logger(SourceFile file, int line);
  Logger(SourceFile file, int line, LogLevel level);
  Logger(SourceFile file, int line, LogLevel level, const char* func);
  Logger(SourceFile file, int line, bool toAbort);
  ~Logger();
  LogStream& stream() { return impl_.stream_; }
  static LogLevel logLevel();
  static void setLogLevel(LogLevel level);
  static void setOutput(std::function<void(const char*, int)>);
  static void setFlush(std::function<void()>);
  static void InitFileLogger(const char *dir, const char* prefix, bool sync);

private:

  class Impl
  {
   public:
    typedef Logger::LogLevel LogLevel;
    Impl(LogLevel level, int old_errno, const SourceFile& file, int line);
    void formatTime();
    void finish();

    Timestamp time_;
    LogStream stream_;
    LogLevel level_;
    int line_;
    SourceFile basename_;
  };

  Impl impl_;

};

extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel Logger::logLevel()
{
  return g_logLevel;
}

///
/// CAUTION: do not write:
///
/// if (good)
///   LOG_INFO << "Good news";
/// else
///   LOG_WARN << "Bad news";
///
/// this expends to
///
/// if (good)
///   if (logging_INFO)
///     logInfoStream << "Good news";
///   else
///     logWarnStream << "Bad news";
///
#define LOG_TRA if (base::Logger::logLevel() <= base::Logger::TRACE) \
  base::Logger(__FILE__, __LINE__, base::Logger::TRACE, __func__).stream()
#define LOG_DBG if (base::Logger::logLevel() <= base::Logger::DEBUG) \
  base::Logger(__FILE__, __LINE__, base::Logger::DEBUG, __func__).stream()
#define LOG_INF if (base::Logger::logLevel() <= base::Logger::INFO) \
  base::Logger(__FILE__, __LINE__).stream()
#define LOG_WAN base::Logger(__FILE__, __LINE__, base::Logger::WARN).stream()
#define LOG_ERR base::Logger(__FILE__, __LINE__, base::Logger::ERROR).stream()
#define LOG_FAT base::Logger(__FILE__, __LINE__, base::Logger::FATAL).stream()

const char* strerror_tl(int savedErrno);

// Taken from glog/logging.h
//
// Check that the input is non NULL.  This very useful in constructor
// initializer lists.

#define CHECK_NOTNULL(val) \
  ::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

// A small helper for CHECK_NOTNULL().
template <typename T>
T* CheckNotNull(Logger::SourceFile file, int line, const char *names, T* ptr)
{
  if (ptr == NULL)
  {
   Logger(file, line, Logger::FATAL).stream() << names;
  }
  return ptr;
}

}

#endif  // BASE_LOGGING_H
