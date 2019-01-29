#include "FlowController.h"
#include "base/common/Time.h"
#include "config/EnvConfig.h"

FlowController::FlowController()
  : FLOW_LIMIT(EnvConfig::GetInstance()->GetInt32(EnvVar::FLOW_CONTROL_LIMIT, 0)) {
}

bool FlowController::TrySubmit(int64_t id) {
  if (FLOW_LIMIT <= 0) {
    return true;
  }

  std::lock_guard<std::mutex> lck(mtx_);
  if (submit_ids_.size() + cancel_ids_.size() + pending_ids_.size() <
      static_cast<size_t>(FLOW_LIMIT)) {
    submit_ids_[id] = base::Now();
    return true;
  } else {
    auto now = base::Now();
    auto rit = pending_ids_.rbegin();
    while (rit != pending_ids_.rend() && (now - *rit) > base::MILLION) ++rit;
    if (rit != pending_ids_.rbegin()) {
      pending_ids_.erase(rit.base(), pending_ids_.end());
      submit_ids_[id] = now;
      return true;
    }
    return false;
  }
}

bool FlowController::TryCancel(int64_t id) {
  if (FLOW_LIMIT <= 0) {
    return true;
  }

  std::lock_guard<std::mutex> lck(mtx_);
  if (submit_ids_.size() + cancel_ids_.size() + pending_ids_.size() <
      static_cast<size_t>(FLOW_LIMIT)) {
    cancel_ids_[id] = base::Now();
    return true;
  } else {
    auto now = base::Now();
    auto rit = pending_ids_.rbegin();
    while (rit != pending_ids_.rend() && (now - *rit) > base::MILLION) ++rit;
    if (rit != pending_ids_.rbegin()) {
      pending_ids_.erase(rit.base(), pending_ids_.end());
      cancel_ids_[id] = now;
      return true;
    }
    return false;
  }
}

void FlowController::ReleaseSubmit(int64_t id) {
  if (FLOW_LIMIT <= 0) {
    return;
  }

  std::lock_guard<std::mutex> lck(mtx_);
  auto it = submit_ids_.find(id);
  if (it != submit_ids_.end()) {
    if ((base::Now() - it->second) < base::MILLION) {
      auto itr = pending_ids_.begin();
      while ((itr != pending_ids_.end()) && (*itr > it->second)) ++itr;
      pending_ids_.insert(itr, it->second);
    }
    submit_ids_.erase(it);
  }
}

void FlowController::ReleaseCancel(int64_t id) {
  if (FLOW_LIMIT <= 0) {
    return;
  }

  std::lock_guard<std::mutex> lck(mtx_);
  auto it = cancel_ids_.find(id);
  if (it != cancel_ids_.end()) {
    if ((base::Now() - it->second) < base::MILLION) {
      auto itr = pending_ids_.begin();
      while ((itr != pending_ids_.end()) && (*itr > it->second)) ++itr;
      pending_ids_.insert(itr, it->second);
    }
    cancel_ids_.erase(it);
  }
}
