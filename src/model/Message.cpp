#include "Message.h"

#include <sys/time.h>

void MsgHeader::SetTime()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  time = tv.tv_sec;
  time = time * 1000000 + tv.tv_usec;
}

void MsgHeader::SetInterval(int idx)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int64_t now = tv.tv_sec;
  now = now * 1000000 + tv.tv_usec;
  interval[idx] = static_cast<int32_t>(now - time);
}
