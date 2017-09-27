#include "CtpMdSpi.h"
#include "CtpMdApi.h"
#include "CtpTraderSpi.h"
#include "base/common/Likely.h"
#include "base/common/CodeConverter.h"
#include "base/logger/Logging.h"
#include "model/Price.h"
#include "model/ProductManager.h"
#include "strategy/ClusterManager.h"
#include "strategy/DeviceManager.h"

#include <cfloat>
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
  // std::string id = CtpTraderSpi::GetInstrumentId(
  //     pDepthMarketData->InstrumentID, pDepthMarketData->ExchangeID);
  const Instrument *inst = ProductManager::GetInstance()->FindId(pDepthMarketData->InstrumentID);
  if (inst)
  {
    auto *dm = ClusterManager::GetInstance()->FindDevice(inst->HedgeUnderlying());
    if (dm == nullptr)
    {
      LOG_ERR << "Can't find device manager for " << inst->HedgeUnderlying();
      return;
    }
    PricePtr price = Message::NewPrice();
    price->header.SetTime();
    price->instrument = inst;
    price->last = (pDepthMarketData->LastPrice < DBL_MAX) ?
      pDepthMarketData->LastPrice : PRICE_UNDEFINED;
    price->bid = (pDepthMarketData->BidPrice1 < DBL_MAX) ?
      pDepthMarketData->BidPrice1 : PRICE_UNDEFINED;
    price->bid_volume = (pDepthMarketData->BidVolume1 < DBL_MAX) ?
      pDepthMarketData->BidVolume1 : VOLUME_UNDEFINED;
    price->ask = (pDepthMarketData->AskPrice1 < DBL_MAX) ?
      pDepthMarketData->AskPrice1 : PRICE_UNDEFINED;
    price->ask_volume = (pDepthMarketData->AskVolume1 < DBL_MAX) ?
      pDepthMarketData->AskVolume1 : VOLUME_UNDEFINED;
    price->open = (pDepthMarketData->OpenPrice < DBL_MAX) ?
      pDepthMarketData->OpenPrice : PRICE_UNDEFINED;
    price->high = (pDepthMarketData->HighestPrice < DBL_MAX) ?
      pDepthMarketData->HighestPrice : PRICE_UNDEFINED;
    price->low = (pDepthMarketData->LowestPrice < DBL_MAX) ?
      pDepthMarketData->LowestPrice : PRICE_UNDEFINED;
    price->close = (pDepthMarketData->ClosePrice < DBL_MAX) ?
      pDepthMarketData->ClosePrice : PRICE_UNDEFINED;
    price->volume = (pDepthMarketData->Volume < DBL_MAX) ?
      pDepthMarketData->Volume : VOLUME_UNDEFINED;
    price->amount = (pDepthMarketData->Turnover < DBL_MAX) ?
      pDepthMarketData->Turnover : PRICE_UNDEFINED;

    if (unlikely(inst->Highest() == PRICE_UNDEFINED || inst->Lowest() == PRICE_UNDEFINED))
    {
      Instrument *p = const_cast<Instrument*>(inst);
      if (pDepthMarketData->UpperLimitPrice < DBL_MAX)
        p->Highest(pDepthMarketData->UpperLimitPrice);
      if (pDepthMarketData->LowerLimitPrice < DBL_MAX)
        p->Lowest(pDepthMarketData->LowerLimitPrice);
      LOG_INF << boost::format("Lowest/Highest price update: %1% - %2%/%3%") %
        p->Id() % p->Lowest() % p->Highest();
    }
    dm->Publish(price);
  }
}

void CtpMdSpi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp)
{}
