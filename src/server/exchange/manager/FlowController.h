#ifndef EXCHANGE_FLOW_CONTROLLER_H
#define EXCHANGE_FLOW_CONTROLLER_H

#include <unordered_map>
#include <list>
#include <mutex>

class FlowController {
 public:
  FlowController();

  bool TrySubmit(int64_t id);
  bool TryCancel(int64_t id);
  void ReleaseSubmit(int64_t id);
  void ReleaseCancel(int64_t id);

 private:
  const int32_t FLOW_LIMIT;
  std::mutex mtx_;
  std::unordered_map<int64_t, int64_t> submit_ids_;
  std::unordered_map<int64_t, int64_t> cancel_ids_;
  std::list<int64_t> pending_ids_;
};

#endif // EXCHANGE_FLOW_CONTROLLER_H
