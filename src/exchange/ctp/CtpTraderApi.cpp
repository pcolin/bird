#include "CtpTraderApi.h"
#include "CtpTraderSpi.h"
#include "base/common/Likely.h"
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"

using namespace base;

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
  api_->SubscribePrivateTopic(THOST_TERT_RESUME);
  api_->SubscribePublicTopic(THOST_TERT_RESUME);
  api_->RegisterFront(const_cast<char*>(
        EnvConfig::GetInstance()->GetString(EnvVar::CTP_TRADE_ADDR).c_str()));
  api_->Init();

  LOG_INF << "Wait until instrument query finished...";
  std::unique_lock<std::mutex> lck(inst_mtx_);
  inst_cv_.wait(lck, [this]{ return inst_ready_; });
}

void CtpTraderApi::Login()
{
  LOG_INF << "Login CTP Trader Api";
  CThostFtdcReqUserLoginField field;
  memset(&field, 0, sizeof(field));
  strcpy(field.BrokerID, "8060");
  strcpy(field.UserID, "99665550");
  strcpy(field.Password, "111111");

  int ret = api_->ReqUserLogin(&field, ++id_);
  if (ret == 0)
  {
    LOG_INF << "Success to send login request.";
  }
  else
  {
    LOG_ERR << "Failed to send login request : " << ret;
  }
}

void CtpTraderApi::Logout()
{
  LOG_INF << "Logout CTP Trader Api";
  CThostFtdcUserLogoutField field;
  memset(&field, 0, sizeof(field));
  strcpy(field.BrokerID, "8060");
  strcpy(field.UserID, "99665550");

  int ret = api_->ReqUserLogout(&field, ++id_);
  if (ret == 0)
  {
    LOG_INF << "Success to send logout request.";
  }
  else
  {
    LOG_ERR << "Failed to send logout request : " << ret;
  }
}

void CtpTraderApi::NewOrder()
{

}

void CtpTraderApi::AmendOrder()
{

}

void CtpTraderApi::PullOrder()
{

}

void CtpTraderApi::PullAll()
{

}

void CtpTraderApi::QueryInstruments()
{
  LOG_INF << "Query instruments";
  CThostFtdcQryInstrumentField field;
  memset(&field, 0, sizeof(field));
  strcpy(field.ExchangeID, "DCE");
  /// strcpy(field.ProductID, "m_o");

  int ret = api_->ReqQryInstrument(&field, id_++);
  if (ret == 0)
  {
    LOG_INF << "Success to send query instrument request";
  }
  else
  {
    LOG_ERR << "Failed to send query instrument request : " << ret;
  }
}

void CtpTraderApi::QueryOrders()
{
  LOG_INF << "Query orders";
  CThostFtdcQryOrderField field;
  memset(&field, 0, sizeof(field));
  strcpy(field.BrokerID, "8060");
  strcpy(field.InvestorID, "99665550");
  strcpy(field.ExchangeID, "DCE");

  int ret = api_->ReqQryOrder(&field, id_++);
  if (ret == 0)
  {
    LOG_INF << "Success to send query orders request";
  }
  else
  {
    LOG_ERR << "Failed to send query orders request : " << ret;
  }
}

void CtpTraderApi::QueryTrades()
{
  LOG_INF << "Query trades";
  CThostFtdcQryTradeField field;
  memset(&field, 0, sizeof(field));
  strcpy(field.BrokerID, "8060");
  strcpy(field.InvestorID, "99665550");
  strcpy(field.ExchangeID, "DCE");

  int ret = api_->ReqQryTrade(&field, id_++);
  if (ret == 0)
  {
    LOG_INF << "Success to send query trades request";
  }
  else
  {
    LOG_ERR << "Failed to send query trades request : " << ret;
  }
}

void CtpTraderApi::QueryPositions()
{
  LOG_INF << "Query positions";
  CThostFtdcQryInvestorPositionField field;
  memset(&field, 0, sizeof(field));
  strcpy(field.BrokerID, "8060");
  strcpy(field.InvestorID, "99665550");

  int ret = api_->ReqQryInvestorPosition(&field, id_++);
  if (ret == 0)
  {
    LOG_INF << "Success to send query positions request";
  }
  else
  {
    LOG_ERR << "Failed to send query positions request : " << ret;
  }
}

void CtpTraderApi::QueryCash()
{
  LOG_INF << "Query cash";
  CThostFtdcQryTradingAccountField field;
  memset(&field, 0, sizeof(field));
  strcpy(field.BrokerID, "8060");
  strcpy(field.InvestorID, "99665550");

  int ret = api_->ReqQryTradingAccount(&field, id_++);
  if (ret == 0)
  {
    LOG_INF << "Success to send query cash request";
  }
  else
  {
    LOG_ERR << "Failed to send query cash request : " << ret;
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
