#include "CtpMdApi.h"
#include "CtpMdSpi.h"
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"
#include "model/ProductManager.h"
#include <boost/format.hpp>

using namespace base;
CtpMdApi::~CtpMdApi()
{
  if (api_) api_->Release();
  if (spi_) delete spi_;
}

void CtpMdApi::Init()
{
  LOG_INF << "Initialize CTP Marketdata Api";
  api_ = CThostFtdcMdApi::CreateFtdcMdApi();
  spi_ = new CtpMdSpi(this);
  api_->RegisterSpi(spi_);
  api_->RegisterFront(const_cast<char*>(
        EnvConfig::GetInstance()->GetString(EnvVar::CTP_MD_ADDR).c_str()));
  api_->Init();
}

void CtpMdApi::Login()
{
  LOG_INF << "Login CTP Marketdata Api";
  CThostFtdcReqUserLoginField field;
  memset(&field, 0, sizeof(field));
  strcpy(field.BrokerID, EnvConfig::GetInstance()->GetString(EnvVar::CTP_BROKER_ID).c_str());
  strcpy(field.UserID, EnvConfig::GetInstance()->GetString(EnvVar::CTP_USER_ID).c_str());
  strcpy(field.Password, EnvConfig::GetInstance()->GetString(EnvVar::CTP_PASSWORD).c_str());

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

void CtpMdApi::Subscribe()
{
  LOG_INF << "Subscribe Marketdata.";
  auto instruments = ProductManager::GetInstance()->FindInstruments
    ([](const Instrument*) { return true; });
  const int n = instruments.size();
  char **insts = new char*[n];
  for (int i = 0; i < n; ++i)
  {
    insts[i] = new char[n+1] {0};
    strncpy(insts[i], instruments[i]->Symbol().c_str(), n);
  }
  int ret = api_->SubscribeMarketData(insts, n);
  if (ret == 0)
  {
    LOG_INF << boost::format("Success to subscibe %1% instruments.") % n;
  }
  else
  {
    LOG_ERR << "Failed to subscribe instruments : " << ret;
  }
}
