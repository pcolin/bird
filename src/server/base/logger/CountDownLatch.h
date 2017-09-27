#ifndef BASE_COUNTDOWNLATCH_H
#define BASE_COUNTDOWNLATCH_H

#include <condition_variable>
#include <boost/noncopyable.hpp>

namespace base
{

class CountDownLatch : boost::noncopyable
{
 public:

  explicit CountDownLatch(int count) : count_(count) {}

  void wait()
  {
    std::unique_lock<std::mutex> lck(mtx_);
    while (count_ > 0)
    {
      cv_.wait(lck);
    }
  }

  void countDown()
  {
    std::unique_lock<std::mutex> lck(mtx_);
    --count_;
    if (count_ == 0)
    {
      cv_.notify_all();
    }
  }

  int getCount() const
  {
    std::unique_lock<std::mutex> lck(mtx_);
    return count_;
  }

 private:
  mutable std::mutex mtx_;
  std::condition_variable cv_;
  int count_;
};

}
#endif  // BASE_COUNTDOWNLATCH_H
