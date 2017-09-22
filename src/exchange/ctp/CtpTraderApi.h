#ifndef CTP_TRADER_API_H
#define CTP_TRADER_API_H

#include <mutex>
#include <condition_variable>
#include "../manager/TraderApi.h"
#include "base/concurrency/blockingconcurrentqueue.h"
#include "3rd_library/ctp/include/ThostFtdcTraderApi.h"

class CtpTraderSpi;
class CtpTraderApi : public TraderApi
{
public:
  CtpTraderApi();
  ~CtpTraderApi();

  virtual void Init();
  virtual void Login();
  virtual void Logout();

  // virtual void PullAll();

  void QueryInstruments();
  void QueryOrders();
  void QueryTrades();
  void QueryPositions();
  void QueryCash();
  void NotifyInstrumentReady();
  // void SetMaxOrderRef(int order_ref)
  // {
  //   order_ref_ = order_ref;
  // }
  void OnUserLogin(int order_ref, int front_id, int session_id);
  OrderPtr FindOrder(int order_ref);
  OrderPtr RemoveOrder(int order_ref);

protected:
  void NewOrder(const OrderPtr &order) override;
  void NewQuote(const OrderPtr &bid, const OrderPtr &ask) override;
  void AmendOrder(const OrderPtr &order) override;
  void AmendQuote(const OrderPtr &bid, const OrderPtr &ask) override;
  void PullOrder(const OrderPtr &order) override;
  void PullQuote(const OrderPtr &bid, const OrderPtr &ask) override;

private:
  void BuildTemplate();

  CThostFtdcTraderApi *api_ = nullptr;
  CtpTraderSpi *spi_ = nullptr;
  std::atomic<int> req_id_ = {0};
  const std::string exchange_;
  const std::string broker_;
  const std::string investor_;
  int order_ref_ = 0;
  CThostFtdcInputOrderField new_order_;
  CThostFtdcInputOrderActionField pull_order_;
  CThostFtdcInputQuoteField new_quote_;
  CThostFtdcInputQuoteActionField pull_quote_;

  std::mutex ref_mtx_;
  std::unordered_map<int, OrderPtr> order_refs_;
  // std::mutex pull_mtx_;
  // std::unordered_set<int> pulling_refs_;
  static const size_t capacity_ = 256;
  moodycamel::BlockingConcurrentQueue<size_t> pulling_ids_;
  std::unique_ptr<std::thread> pulling_thread_;

  std::mutex inst_mtx_;
  std::condition_variable inst_cv_;
  bool inst_ready_ = false;
};

#endif
