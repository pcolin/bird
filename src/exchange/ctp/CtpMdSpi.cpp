#include "CtpMdSpi.h"
#include "CtpMdApi.h"
#include "base/common/Likely.h"
#include "base/common/CodeConverter.h"
#include "base/logger/Logging.h"

#include <boost/format.hpp>

using namespace base;
CtpMdSpi::CtpMdSpi(CtpMdApi *api)
  : api_(api)
{}

void CtpMdSpi::OnFrontConnected()
{
  LOG_INF << "OnFrontConnected.";
  api_->Login();
}

void CtpMdSpi::OnHeartBeatWarning(int nTimeLapse)
{
  LOG_WAN << boost::format("OnHeartBeatWarning %1%s.") % nTimeLapse;
}

void CtpMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
  if (pRspInfo && pRspInfo->ErrorID != 0)
  {
    LOG_ERR << boost::format("Failed to login ctp md : %1%(%2%).") %
      pRspInfo->ErrorID % GB2312ToUtf8(pRspInfo->ErrorMsg);
    return;
  }
  LOG_INF << boost::format("Success to login ctp md %1%") % pRspUserLogin->UserID;
  api_->Subscribe();
}

void CtpMdSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void CtpMdSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void CtpMdSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void CtpMdSpi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void CtpMdSpi::OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void CtpMdSpi::OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void CtpMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
  if (unlikely(!pDepthMarketData))
  {
    LOG_WAN << "OnRtnDepthMarketData : Null parameter";
    return;
  }

  LOG_DBG << boost::format("OnRtnDepthMarketData : TradingDay(%1%), InstrumentID(%2%), "
      "ExchangeID(%3%), ExchangeInstID(%4%), LastPrice(%5%), PreSettlementPrice(%6%), "
      "PreClosePrice(%7%), OpenPrice(%8%), HighestPrice(%9%), LowestPrice(%10%), Volume(%11%), "
      "Turnover(%12%), OpenInterest(%13%), ClosePrice(%14%), SettlementPrice(%15%), "
      "UpperLimitPrice(%16%), LowerLimitPrice(%17%), BidPrice1(%18%), BidVolume1(%19%)"
      "AskPrice1(%20%), AskVolume1(%21%), ActionDay(%22%)")
      % pDepthMarketData->TradingDay % pDepthMarketData->InstrumentID % pDepthMarketData->ExchangeID
      % pDepthMarketData->ExchangeInstID % pDepthMarketData->LastPrice
      % pDepthMarketData->PreSettlementPrice % pDepthMarketData->PreClosePrice
      % pDepthMarketData->OpenPrice % pDepthMarketData->HighestPrice % pDepthMarketData->LowestPrice
      % pDepthMarketData->Volume % pDepthMarketData->Turnover % pDepthMarketData->OpenInterest
      % pDepthMarketData->ClosePrice % pDepthMarketData->SettlementPrice
      % pDepthMarketData->UpperLimitPrice % pDepthMarketData->LowerLimitPrice
      % pDepthMarketData->BidPrice1 % pDepthMarketData->BidVolume1 % pDepthMarketData->AskPrice1
      % pDepthMarketData->AskVolume1 % pDepthMarketData->ActionDay;
}

void CtpMdSpi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp)
{}
