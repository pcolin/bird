#include "CtpTraderApi.h"
#include "CtpTraderSpi.h"
#include "base/common/Likely.h"
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"
#include "model/PositionManager.h"
#include "model/OrderManager.h"

#include <boost/format.hpp>

using namespace base;

CtpTraderApi::CtpTraderApi()
  : exchange_(EnvConfig::GetInstance()->GetString(EnvVar::EXCHANGE)),
    broker_(EnvConfig::GetInstance()->GetString(EnvVar::CTP_BROKER_ID)),
    investor_(EnvConfig::GetInstance()->GetString(EnvVar::CTP_INVESTOR_ID)),
    pulling_ids_(capacity_)
{}

CtpTraderApi::~CtpTraderApi()
{
  if (api_) api_->Release();
  if (spi_) delete spi_;
}

void CtpTraderApi::Init()
{
  LOG_INF << "Initialize CTP trader api";
  api_ = CThostFtdcTraderApi::CreateFtdcTraderApi();
  spi_ = new CtpTraderSpi(this);
  api_->RegisterSpi(spi_);
  // api_->SubscribePrivateTopic(THOST_TERT_RESUME);
  // api_->SubscribePublicTopic(THOST_TERT_RESUME);
  api_->SubscribePrivateTopic(THOST_TERT_QUICK);
  api_->SubscribePublicTopic(THOST_TERT_QUICK);
  api_->RegisterFront(const_cast<char*>(
        EnvConfig::GetInstance()->GetString(EnvVar::CTP_TRADE_ADDR).c_str()));
  api_->Init();

  LOG_INF << "Wait until instrument query finished...";
  std::unique_lock<std::mutex> lck(inst_mtx_);
  inst_cv_.wait(lck, [this]{ return inst_ready_; });
  // StartRequestWork();
  TraderApi::Init();
  pulling_thread_ = std::make_unique<std::thread>([&]()
    {
      LOG_INF << "Start thread to pull saved orders...";
      size_t ids[capacity_];
      while (true)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        size_t cnt = pulling_ids_.wait_dequeue_bulk(ids, capacity_);
        LOG_INF << boost::format("Get %1% saved orders") % cnt;
        for (size_t i = 0; i < cnt; ++i)
        {
          auto ord = OrderManager::GetInstance()->FindActiveOrder(ids[i]);
          if (ord)
          {
            if (!ord->counter_id.empty())
              PullOrder(ord);
            else
              pulling_ids_.enqueue(ids[i]);
          }
        }
      }
    });
}

void CtpTraderApi::Login()
{
  const std::string user = EnvConfig::GetInstance()->GetString(EnvVar::CTP_USER_ID);
  const std::string pwd = EnvConfig::GetInstance()->GetString(EnvVar::CTP_PASSWORD);
  LOG_INF << boost::format("Login CTP Trader Api(%1%,%2%)") % user % pwd;
  CThostFtdcReqUserLoginField field;
  memset(&field, 0, sizeof(field));
  strcpy(field.BrokerID, broker_.c_str());
  strcpy(field.UserID, user.c_str());
  strcpy(field.Password, pwd.c_str());

  while (int ret = api_->ReqUserLogin(&field, req_id_++))
  {
    LOG_ERR << boost::format("Failed to send login request(%1%), retry after 1s") % ret;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  LOG_INF << "Success to send login request.";
}

void CtpTraderApi::Logout()
{
  LOG_INF << "Logout CTP Trader Api";
  CThostFtdcUserLogoutField field;
  memset(&field, 0, sizeof(field));
  strcpy(field.BrokerID, broker_.c_str());
  strcpy(field.UserID, investor_.c_str());

  while (int ret = api_->ReqUserLogout(&field, req_id_++))
  {
    LOG_ERR << boost::format("Failed to send logout request(%1%), retry after 1s") % ret;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  LOG_INF << "Success to send logout request.";
}

void CtpTraderApi::NewOrder(const OrderPtr &order)
{
  if (unlikely(!order)) return;

  /// Emergency to be done...

  if (unlikely(!protector_.TryAdd(order)))
  {
    order->note = "wash trade";
    RejectOrder(order);
    return;
  }

  CThostFtdcInputOrderField field(new_order_);
  strcpy(field.InstrumentID, order->instrument->Symbol().c_str());
  // const std::string order_ref_str = std::to_string(++order_ref_);
  // strcpy(field.OrderRef, order_ref_str.c_str());
  sprintf(field.OrderRef, "%012d", ++order_ref_);
  field.Direction = order->IsBid() ? THOST_FTDC_D_Buy : THOST_FTDC_D_Sell;

  if (order->IsOpen())
  {
    if (PositionManager::GetInstance()->TryFreeze(order))
    {
      order->side = order->IsBid() ? Side::BuyCover : Side::SellCover;
      field.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
    }
    else
    {
      field.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
    }
  }
  else
  {
    field.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
  }

  if (order->time_condition == TimeCondition::IOC)
    field.TimeCondition = THOST_FTDC_TC_IOC;

  field.LimitPrice = order->price;
  field.VolumeTotalOriginal = order->volume;

  int id = req_id_++;
  int ret = api_->ReqOrderInsert(&field, id);
  if (ret == 0)
  {
    // auto ord = Message::NewOrder(order);
    // ord->counter_id = field.OrderRef;
    // ord->status = OrderStatus::Submitted;
    // ord->header.SetInterval(0);
    // OnOrderResponse(ord);
    order->header.SetInterval(0);
    order_refs_[order_ref_] = order;
    LOG_INF << "Success to send order request " << id;
    // LOG_INF << boost::format("Field: %1%,%2%,%3%,%4%,%5%,%6%,%7%,%8%,%9%,%10%,%11%,%12%,%13%,%14%,"
    //     "%15%,%16%") %
    //   field.BrokerID % field.InvestorID % field.InstrumentID % field.OrderRef % field.Direction %
    //   field.CombOffsetFlag[0] % field.CombHedgeFlag[0] % field.VolumeTotalOriginal %
    //   field.VolumeCondition % field.MinVolume % field.ForceCloseReason % field.IsAutoSuspend %
    //   field.UserForceClose % field.OrderPriceType % field.LimitPrice % field.TimeCondition;
  }
  else
  {
    LOG_ERR << "Failed to send order request: " << ret;
    order->note = std::move((boost::format("Send fail %1%") % ret).str());
    RejectOrder(order);
  }
}

void CtpTraderApi::NewQuote(const OrderPtr &bid, const OrderPtr &ask)
{
}

void CtpTraderApi::AmendOrder(const OrderPtr &order)
{
}

void CtpTraderApi::AmendQuote(const OrderPtr &bid, const OrderPtr &ask)
{
}

void CtpTraderApi::PullOrder(const OrderPtr &order)
{
  if (unlikely(!order)) return;

  if (unlikely(order->IsInactive()))
  {
    LOG_INF << "Order is completed.";
    return;
  }

  std::string counter_id = order->counter_id;
  if (counter_id.empty())
  {
    auto ord = OrderManager::GetInstance()->FindActiveOrder(order->id);
    if (ord)
    {
      if (ord->counter_id.empty())
      {
        LOG_INF << boost::format("Counter ID of %1% is unavailable, saved...") % order->id;
        pulling_ids_.enqueue(order->id);
        return;
      }
      else
      {
        LOG_INF << "Original counter id is empty, but find one";
        counter_id = ord->counter_id;
      }
    }
    else
    {
      LOG_INF << "Order is completed.";
      return;
    }
  }

  CThostFtdcInputOrderActionField field(pull_order_);
  strcpy(field.InstrumentID, order->instrument->Symbol().c_str());
  strcpy(field.OrderRef, counter_id.c_str());
  int id = req_id_++;
  int ret = api_->ReqOrderAction(&field, id);
  if (ret == 0)
  {
    LOG_INF << boost::format("Success to send order action request %1% (InstrumentID:%2%, FrontID:"
        "%3%, SessionID:%4%, OrderRef:%5%)") %
      id % field.InstrumentID % field.FrontID % field.SessionID % field.OrderRef;
  }
  else
  {
    pulling_ids_.enqueue(order->id);
    LOG_ERR << "Failed to send order action request: " << ret;
  }
}

void CtpTraderApi::PullQuote(const OrderPtr &bid, const OrderPtr &ask)
{
}

// void CtpTraderApi::PullAll()
// {
// }

void CtpTraderApi::QueryInstruments()
{
  LOG_INF << "Query instruments of " << exchange_;
  CThostFtdcQryInstrumentField field;
  memset(&field, 0, sizeof(field));
  strcpy(field.ExchangeID, exchange_.c_str());
  /// strcpy(field.ProductID, "m_o");

  while (int ret = api_->ReqQryInstrument(&field, req_id_++))
  {
    LOG_ERR << boost::format("Failed to send query instrument request(%1%), retry after 1s") % ret;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  LOG_INF << "Success to send query instrument request";
}

void CtpTraderApi::QueryOrders()
{
  LOG_TRA << "Query orders";
  CThostFtdcQryOrderField field;
  memset(&field, 0, sizeof(field));
  strcpy(field.BrokerID, broker_.c_str());
  strcpy(field.InvestorID, investor_.c_str());
  strcpy(field.ExchangeID, exchange_.c_str());

  while (int ret = api_->ReqQryOrder(&field, req_id_++))
  {
    LOG_ERR << boost::format("Failed to send query order request(%1%), retry after 1s") % ret;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  LOG_INF << "Success to send query orders request";
}

void CtpTraderApi::QueryTrades()
{
  LOG_TRA << "Query trades";
  CThostFtdcQryTradeField field;
  memset(&field, 0, sizeof(field));
  strcpy(field.BrokerID, broker_.c_str());
  strcpy(field.InvestorID, investor_.c_str());
  strcpy(field.ExchangeID, exchange_.c_str());

  while (int ret = api_->ReqQryTrade(&field, req_id_++))
  {
    LOG_ERR << boost::format("Failed to send query trade request(%1%), retry after 1s") % ret;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  LOG_INF << "Success to send query trades request";
}

void CtpTraderApi::QueryPositions()
{
  LOG_TRA << "Query positions";
  CThostFtdcQryInvestorPositionField field;
  memset(&field, 0, sizeof(field));
  strcpy(field.BrokerID, broker_.c_str());
  strcpy(field.InvestorID, investor_.c_str());

  while (int ret = api_->ReqQryInvestorPosition(&field, req_id_++))
  {
    LOG_ERR << boost::format("Failed to send query position request(%1%), retry after 1s") % ret;
    std::this_thread::sleep_for(std::chrono::seconds(2));
  }
  LOG_INF << "Success to send query positions request";
}

void CtpTraderApi::QueryCash()
{
  LOG_TRA << "Query cash";
  CThostFtdcQryTradingAccountField field;
  memset(&field, 0, sizeof(field));
  strcpy(field.BrokerID, broker_.c_str());
  strcpy(field.InvestorID, investor_.c_str());

  // while (int ret = api_->ReqQryTradingAccount(&field, req_id_++))
  // {
  //   LOG_ERR << boost::format("Failed to send query cash request(%1%), retry after 1s") % ret;
  //   std::this_thread::sleep_for(std::chrono::seconds(1));
  // }
  int ret = api_->ReqQryTradingAccount(&field, req_id_++);
  if (ret)
  {
    LOG_ERR << boost::format("Failed to send query cash request(return code = %1%)") % ret;
  }
  else
  {
    LOG_INF << "Success to send query cash request";
  }
}

void CtpTraderApi::NotifyInstrumentReady()
{
  {
    std::lock_guard<std::mutex> lck(inst_mtx_);
    inst_ready_ = true;
  }
  inst_cv_.notify_one();
}

void CtpTraderApi::OnUserLogin(int order_ref, int front_id, int session_id)
{
  order_ref_ = order_ref;

  memset(&new_order_, 0, sizeof(new_order_));
  strcpy(new_order_.BrokerID, broker_.c_str());
  strcpy(new_order_.InvestorID, investor_.c_str());
  new_order_.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
  new_order_.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
  new_order_.TimeCondition = THOST_FTDC_TC_GFD;
  // new_order_.IsAutoSuspend = 0;
  // new_order_.IsSwapOrder = 0;
  // new_order_.UserForceClose = 0;
  new_order_.VolumeCondition = THOST_FTDC_VC_AV;
  // new_order_.MinVolume = 0;
  // new_order_.StopPrice = 0;
  new_order_.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
  new_order_.ContingentCondition = THOST_FTDC_CC_Immediately; /// Must be set!!!

  memset(&pull_order_, 0, sizeof(pull_order_));
  pull_order_.FrontID = front_id;
  pull_order_.SessionID = session_id;
  strcpy(pull_order_.BrokerID, broker_.c_str());
  strcpy(pull_order_.InvestorID, investor_.c_str());
  pull_order_.ActionFlag = THOST_FTDC_AF_Delete;

  memset(&new_quote_, 0, sizeof(new_quote_));
  strcpy(new_quote_.BrokerID, broker_.c_str());
  strcpy(new_quote_.InvestorID, investor_.c_str());
  new_quote_.BidHedgeFlag = THOST_FTDC_HF_Speculation;
  new_quote_.AskHedgeFlag = THOST_FTDC_HF_Speculation;

  memset(&pull_quote_, 0, sizeof(pull_quote_));
  strcpy(pull_quote_.BrokerID, broker_.c_str());
  strcpy(pull_quote_.InvestorID, investor_.c_str());
  strcpy(pull_quote_.ExchangeID, exchange_.c_str());
  pull_quote_.ActionFlag = THOST_FTDC_AF_Delete;
}

OrderPtr CtpTraderApi::FindOrder(int order_ref)
{
  std::lock_guard<std::mutex> lck(ref_mtx_);
  auto it = order_refs_.find(order_ref);
  return it != order_refs_.end() ? it->second : nullptr;
}

OrderPtr CtpTraderApi::RemoveOrder(int order_ref)
{
  std::lock_guard<std::mutex> lck(ref_mtx_);
  auto it = order_refs_.find(order_ref);
  if (it != order_refs_.end())
  {
    OrderPtr ord = it->second;
    order_refs_.erase(it);
    return ord;
  }
  return nullptr;
}

void CtpTraderApi::BuildTemplate()
{
  memset(&new_order_, 0, sizeof(new_order_));
  strcpy(new_order_.BrokerID, broker_.c_str());
  strcpy(new_order_.InvestorID, investor_.c_str());
  new_order_.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
  new_order_.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
  new_order_.TimeCondition = THOST_FTDC_TC_GFD;
  // new_order_.IsAutoSuspend = 0;
  // new_order_.IsSwapOrder = 0;
  // new_order_.UserForceClose = 0;
  new_order_.VolumeCondition = THOST_FTDC_VC_AV;
  new_order_.MinVolume = 0;
  // new_order_.StopPrice = 0;
  new_order_.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
  new_order_.ContingentCondition = THOST_FTDC_CC_Immediately; /// Must be set!!!

  memset(&pull_order_, 0, sizeof(pull_order_));
  strcpy(pull_order_.BrokerID, broker_.c_str());
  strcpy(pull_order_.InvestorID, investor_.c_str());
  strcpy(pull_order_.ExchangeID, exchange_.c_str());
  pull_order_.ActionFlag = THOST_FTDC_AF_Delete;

  memset(&new_quote_, 0, sizeof(new_quote_));
  strcpy(new_quote_.BrokerID, broker_.c_str());
  strcpy(new_quote_.InvestorID, investor_.c_str());
  new_quote_.BidHedgeFlag = THOST_FTDC_HF_Speculation;
  new_quote_.AskHedgeFlag = THOST_FTDC_HF_Speculation;

  memset(&pull_quote_, 0, sizeof(pull_quote_));
  strcpy(pull_quote_.BrokerID, broker_.c_str());
  strcpy(pull_quote_.InvestorID, investor_.c_str());
  strcpy(pull_quote_.ExchangeID, exchange_.c_str());
  pull_quote_.ActionFlag = THOST_FTDC_AF_Delete;
}
