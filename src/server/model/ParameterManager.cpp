#include "ParameterManager.h"
#include "ProductManager.h"

ParameterManager* ParameterManager::GetInstance()
{
  static ParameterManager manager;
  return &manager;
}

void ParameterManager::Init()
{

}

bool ParameterManager::GetInterestRate(const boost::gregorian::date &date, double &rate)
{
  std::lock_guard<std::mutex> lck(interest_rates_mtx_);
  if (!interest_rates_.empty())
  {
    int32_t days = (date - boost::gregorian::day_clock::local_day()).days() + 1;
    auto it = interest_rates_.upper_bound(days);
    if (it == interest_rates_.begin())
    {
      rate = it->second;
    }
    else if (it != interest_rates_.end())
    {
      int32_t up_days = it->first;
      double up_rates = it->second;
      --it;
      rate = it->second + (up_rates - it->second) * (days - it->first) / (up_days - it->first);
    }
    else
    {
      rate = interest_rates_.rbegin()->second;
    }
    return true;
  }
  return false;
}

bool ParameterManager::GetSSRate(const Instrument *underlying, const boost::gregorian::date &date, double rate)
{
  std::lock_guard<std::mutex> lck(ssrates_mtx_);
  auto it = ssrates_.find(underlying);
  if (it != ssrates_.end())
  {
    auto it1 = it->second->find(date);
    if (it1 != it->second->end())
    {
      rate = it1->second;
      return true;
    }
  }
  return false;
}

bool ParameterManager::GetVolatility(const Instrument *instrument, double &volatility)
{
  std::lock_guard<std::mutex> lck(volatilities_mtx_);
  auto it = volatilities_.find(instrument);
  if (it != volatilities_.end())
  {
    volatility = it->second;
    return true;
  }
  return false;
}

bool ParameterManager::GetDestriker(const Instrument *instrument, double &destriker)
{
  std::lock_guard<std::mutex> lck(destrikers_mtx_);
  auto it = destrikers_.find(instrument);
  if (it != destrikers_.end())
  {
    destriker = it->second;
    return true;
  }
  return false;
}

bool ParameterManager::GetElastic(const Instrument *instrument, double &elastic)
{
  std::lock_guard<std::mutex> lck(elastics_mtx_);
  auto it = elastics_.find(instrument);
  if (it != elastics_.end())
  {
    elastic = it->second;
    return true;
  }
  return false;
}

void ParameterManager::OnInterestRateReq(const std::shared_ptr<Proto::InterestRateReq> &req)
{
  if (req->type() == Proto::RequestType::Set)
  {
    std::lock_guard<std::mutex> lck(interest_rates_mtx_);
    interest_rates_.clear();
    for (auto &r : req->rates())
    {
      interest_rates_[r.days()] = r.rate();
    }
  }
}

void ParameterManager::OnSSRateReq(const std::shared_ptr<Proto::SSRateReq> &req)
{
  auto type = req->type();
  if (type == Proto::RequestType::Set)
  {
    auto *inst = ProductManager::GetInstance()->FindId(req->instrument());
    if (inst)
    {
      std::lock_guard<std::mutex> lck(ssrates_mtx_);
      auto it = ssrates_.find(inst);
      if (it != ssrates_.end())
      {
        for (auto &r : req->rates())
        {
          auto date = boost::gregorian::from_undelimited_string(r.date());
          (*it->second)[date] = r.rate();
        }
      }
    }
  }
  else if (type == Proto::RequestType::Del)
  {
    auto *inst = ProductManager::GetInstance()->FindId(req->instrument());
    if (inst)
    {
      std::lock_guard<std::mutex> lck(ssrates_mtx_);
      auto it = ssrates_.find(inst);
      if (it != ssrates_.end())
      {
        for (auto &r : req->rates())
        {
          auto date = boost::gregorian::from_undelimited_string(r.date());
          it->second->erase(date);
        }
      }
    }
  }
}

void ParameterManager::OnVolatilityReq(const std::shared_ptr<Proto::VolatilityReq> &req)
{
  auto type = req->type();
  if (type == Proto::RequestType::Set)
  {
    std::lock_guard<std::mutex> lck(volatilities_mtx_);
    for (auto &v : req->volatilities())
    {
      auto *inst = ProductManager::GetInstance()->FindId(v.instrument());
      if (inst)
      {
        volatilities_[inst] = v.volatility();
      }
    }
  }
  else if (type == Proto::RequestType::Del)
  {
    std::lock_guard<std::mutex> lck(volatilities_mtx_);
    for (auto &v : req->volatilities())
    {
      auto *inst = ProductManager::GetInstance()->FindId(v.instrument());
      if (inst)
      {
        volatilities_.erase(inst);
      }
    }
  }
}

void ParameterManager::OnDestrikerReq(const std::shared_ptr<Proto::DestrikerReq> &req)
{
  auto type = req->type();
  if (type == Proto::RequestType::Set)
  {
    std::lock_guard<std::mutex> lck(destrikers_mtx_);
    for (auto &d : req->destrikers())
    {
      auto *inst = ProductManager::GetInstance()->FindId(d.instrument());
      if (inst)
      {
        destrikers_[inst] = d.destriker();
      }
    }
  }
  else if (type == Proto::RequestType::Del)
  {
    std::lock_guard<std::mutex> lck(destrikers_mtx_);
    for (auto &d : req->destrikers())
    {
      auto *inst = ProductManager::GetInstance()->FindId(d.instrument());
      if (inst)
      {
        destrikers_.erase(inst);
      }
    }
  }
}

void ParameterManager::OnElasticReq(const std::shared_ptr<Proto::ElasticReq> &req)
{
  auto type = req->type();
  if (type == Proto::RequestType::Set)
  {
    std::lock_guard<std::mutex> lck(elastics_mtx_);
    for (auto &e : req->elastics())
    {
      auto *inst = ProductManager::GetInstance()->FindId(e.instrument());
      if (inst)
      {
        elastics_[inst] = e.elastic();
      }
    }
  }
  else if (type == Proto::RequestType::Del)
  {
    std::lock_guard<std::mutex> lck(elastics_mtx_);
    for (auto &e : req->elastics())
    {
      auto *inst = ProductManager::GetInstance()->FindId(e.instrument());
      if (inst)
      {
        elastics_.erase(inst);
      }
    }
  }
}
