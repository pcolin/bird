#ifndef BASE_TIME_H
#define BASE_TIME_H

#include <sys/time.h>
// #include "Itoa.h"

namespace base
{
  inline int64_t Now()
  {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
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
}

#endif
