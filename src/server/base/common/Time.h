#ifndef BASE_TIME_H
#define BASE_TIME_H

#include <sys/time.h>
#include <cstring>
// #include "Itoa.h"

namespace base {

const int64_t MILLION = 1e+6;

inline int64_t Now() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * MILLION + tv.tv_usec;
}

inline void TimeToString(int64_t time, char* str, size_t n) {
  time_t seconds = time / MILLION;
  struct tm* t = localtime(&seconds);
  size_t cnt = strftime(str, n, "%Y-%m-%d %H:%M:%S", t);
  snprintf(str + cnt, n - cnt, ".%06ld", time % MILLION);
}

/// 2018-07-17 09:36:01.001234
inline int64_t StringToTime(const char* str) {
  struct tm t;
  memset(&t, 0, sizeof(t));
  t.tm_year = atoi(str) - 1900;
  t.tm_mon = atoi(str + 5) - 1;
  t.tm_mday = atoi(str + 8);
  t.tm_hour = atoi(str + 11);
  t.tm_min = atoi(str + 14);
  t.tm_sec = atoi(str + 17);
  return mktime(&t) * MILLION + atoi(str + 20);
}

/// 20180717 09:36:01
inline int64_t StringToTime(const char* date, const char* time) {
  struct tm t;
  memset(&t, 0, sizeof(t));
  int d = atoi(date);
  t.tm_year = d / 10000 - 1900;
  t.tm_mon = (d / 100) % 100 - 1;
  t.tm_mday = d % 100;
  t.tm_hour = atoi(time);
  t.tm_min = atoi(time + 3);
  t.tm_sec = atoi(time + 6);
  return mktime(&t) * MILLION;
}
// inline std::string ToIsoString(int year, int month, int day)
// {
//   char buffer[9];
//   convert(buffer, year);
//   if (month > 9)
//   {
//     convert(buffer + 4, month);
//   }
//   else
//   {
//     buffer[4] = '0';
//     convert(buffer + 5, month);
//   }
//   if (day > 9)
//   {
//     convert(buffer + 6, day);
//   }
//   else
//   {
//     buffer[6] = '0';
//     convert(buffer + 7, day);
//   }
//   return buffer;
// }
} // namespace base

#endif // BASE_TIME_H
