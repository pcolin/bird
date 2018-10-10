#include "Logging.h"
#include <sys/syscall.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <sstream>
#include "SyncLogging.h"
#include "AsyncLogging.h"
#include "../common/Likely.h"
#include "Timestamp.h"
#include <boost/format.hpp>

namespace base {

thread_local int t_cachedTid = 0;
thread_local char t_tidString[32];
thread_local int t_tidStringLength = 6;

inline void tid() {
  if (unlikely(t_cachedTid == 0)) {
    t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
    t_tidStringLength = snprintf(t_tidString, sizeof(t_tidString), "%5d ", t_cachedTid);
  }
}

thread_local char t_errnobuf[512];
thread_local char t_time[32];
thread_local time_t t_lastSecond;

const char* strerror_tl(int savedErrno) {
  return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}

Logger::LogLevel initLogLevel() {
  if (::getenv("BASE_LOG_TRACE")) {
    return Logger::TRACE;
  } else if (::getenv("BASE_LOG_DEBUG")) {
    return Logger::DEBUG;
  } else {
    return Logger::INFO;
  }
}

Logger::LogLevel g_logLevel = initLogLevel();

const char* LogLevelName[Logger::NUM_LOG_LEVELS] =
    {
      "TRA ",
      "DBG ",
      "INF ",
      "PUB ",
      "WAN ",
      "ERR ",
      "FAT ",
    };

// helper class for known string length at compile time
class T {
 public:
  T(const char* str, unsigned len)
      :str_(str), len_(len) {
    assert(strlen(str) == len_);
  }

  const char* str_;
  const unsigned len_;
};

inline LogStream& operator<<(LogStream& s, T v) {
  s.append(v.str_, v.len_);
  return s;
}

inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v) {
  s.append(v.data_, v.size_);
  return s;
}

void defaultOutput(const char* msg, int len) {
  size_t n = fwrite(msg, 1, len, stdout);
  //FIXME check n
  (void)n;
}

void defaultFlush() { fflush(stdout); }

std::function<void(const char*, int)> g_output = defaultOutput;
std::function<void(Logger::LogLevel lvl, const char*, int)> g_net_output;
std::function<void()> g_flush = defaultFlush;

Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file, int line)
    : time_(Timestamp::now()),
      stream_(),
      level_(level),
      line_(line),
      basename_(file) {
  formatTime();
  tid();
  stream_ << T(t_tidString, t_tidStringLength);
  stream_ << T(LogLevelName[level], 4);
  if (savedErrno != 0) {
    stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
  }
}

void Logger::Impl::formatTime() {
  int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
  time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / Timestamp::kMicroSecondsPerSecond);
  int microseconds = static_cast<int>(microSecondsSinceEpoch % Timestamp::kMicroSecondsPerSecond);
  if (seconds != t_lastSecond) {
    t_lastSecond = seconds;
    struct tm tm_time;
    seconds += 8 * 3600;  /// beijing timezone
    gmtime_r(&seconds, &tm_time);
    int len = snprintf(t_time, sizeof(t_time), "%02d:%02d:%02d",
                       tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    assert(len == 8); (void)len;
  }

  Fmt us(".%06d ", microseconds);
  assert(us.length() == 8);
  stream_ << T(t_time, 8) << T(us.data(), 8);
}

void Logger::Impl::finish() { stream_ << " - " << basename_ << ':' << line_ << '\n'; }

Logger::Logger(SourceFile file, int line)
    : impl_(INFO, 0, file, line) {}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
    : impl_(level, 0, file, line) {
  impl_.stream_ << func << ' ';
}

Logger::Logger(SourceFile file, int line, LogLevel level)
    : impl_(level, 0, file, line) {}

Logger::Logger(SourceFile file, int line, bool toAbort)
    : impl_(toAbort?FATAL:ERROR, errno, file, line) {}

Logger::~Logger() {
  const LogStream::Buffer& buf(Stream().buffer());
  if (unlikely(impl_.level_ > INFO && g_net_output)) {
    const int head_len = 26;
    g_net_output(impl_.level_, buf.data() + head_len, buf.length() - head_len);
  }
  impl_.finish();
  g_output(buf.data(), buf.length());
  if (unlikely(impl_.level_ == FATAL)) {
    g_flush();
    abort();
  }
}

void Logger::SetLogLevel(Logger::LogLevel level) { g_logLevel = level; }

void Logger::SetOutput(std::function<void(const char*, int)> out) { g_output = out; }

void Logger::SetNetOutput(std::function<void(LogLevel lvl, const char*, int)> out) {
  g_net_output = out;
}

void Logger::SetFlush(std::function<void()> flush) { g_flush = flush; }

void Logger::InitFileLogger(const char* dir, const char* prefix, bool sync) {
  boost::format fmt("%1%-%2%-%3%.log");
  std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  fmt % prefix % std::put_time(localtime(&now), "%Y%m%d-%H%M%S");
  fmt % static_cast<pid_t>(::syscall(SYS_gettid));
  if (sync) {
    static SyncLogging logger(dir, fmt.str());
    Logger::SetOutput([&](const char* msg, int len) { logger.append(msg, len); });
  } else {
    static AsyncLogging logger(dir, fmt.str());
    Logger::SetOutput([&](const char* msg, int len) { logger.append(msg, len); });
    logger.start();
  }
}

} // namespace base
