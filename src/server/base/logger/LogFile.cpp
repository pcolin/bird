#include "LogFile.h"
#include "Logging.h"


#include <assert.h>
#include <stdio.h>

using namespace base;

LogFile::LogFile(const std::string& file)
  : file_(::fopen(file.c_str(), "ae"))
{
  assert(file_);
  ::setbuffer(file_, buffer_, sizeof(buffer_));
}

LogFile::~LogFile()
{
  ::fclose(file_);
}

void LogFile::append(const char* logline, int len)
{
  size_t n = ::fwrite_unlocked(logline, 1, len, file_);
  size_t remain = len - n;
  while (remain > 0)
  {
    size_t x = ::fwrite_unlocked(logline + n, 1, remain, file_);
    if (x == 0)
    {
      int err = ferror(file_);
      if (err)
      {
        fprintf(stderr, "AppendFile::append() failed %s\n", strerror_tl(err));
      }
      break;
    }
    n += x;
    remain = len - n;
  }
}

void LogFile::flush()
{
  ::fflush(file_);
}
