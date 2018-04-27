#ifndef BASE_LOGFILE_H
#define BASE_LOGFILE_H

#include <stdio.h>
#include <string>
#include "boost/noncopyable.hpp"

namespace base
{

class LogFile : boost::noncopyable
{
 public:
  LogFile(const std::string& file);
  ~LogFile();

  void append(const char* logline, int len);
  void flush();

 private:
  // const std::string basename_;
  FILE *file_;
  char buffer_[64*1024];

};

}
#endif  // BASE_LOGFILE_H
