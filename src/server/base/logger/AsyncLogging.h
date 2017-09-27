#ifndef BASE_ASYNC_LOGGING_H
#define BASE_ASYNC_LOGGING_H

#include <vector>
#include <thread>
#include "LogStream.h"
#include "CountDownLatch.h"

namespace base
{

class AsyncLogging : boost::noncopyable
{
 public:

  AsyncLogging(const std::string& path,
               const std::string& name,
               int flushInterval = 3);

  ~AsyncLogging()
  {
    if (running_)
    {
      stop();
    }
  }

  void append(const char* logline, int len);

  void start()
  {
    running_ = true;
    thread_ = std::make_unique<std::thread>(std::bind(&AsyncLogging::threadFunc, this));
    latch_.wait();
  }

  void stop()
  {
    if (thread_)
    {
      running_ = false;
      cond_.notify_all();
      thread_->join();
    }
  }

 private:

  // declare but not define, prevent compiler-synthesized functions
  AsyncLogging(const base::AsyncLogging&);  // ptr_container
  void operator=(const base::AsyncLogging&);  // ptr_container

  void threadFunc();

  typedef detail::FixedBuffer<detail::kLargeBuffer> Buffer;
  typedef std::unique_ptr<Buffer> BufferPtr;
  typedef std::vector<BufferPtr> BufferVector;

  const int flushInterval_;
  bool running_;
  std::string filename_;
  std::unique_ptr<std::thread> thread_;
  CountDownLatch latch_;
  std::mutex mutex_;
  std::condition_variable cond_;
  BufferPtr currentBuffer_;
  BufferPtr nextBuffer_;
  BufferVector buffers_;
};

}
#endif  // BASE_ASYNCLOGGING_H
