#include "CtpMdSpi.h"
#include <cfloat>
#include "CtpMdApi.h"
#include "CtpTraderSpi.h"
#include "base/common/Likely.h"
#include "base/common/CodeConverter.h"
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"
#include "model/Price.h"
#include "model/ProductManager.h"
#include "strategy/ClusterManager.h"
#include "strategy/DeviceManager.h"
#include "boost/format.hpp"

using namespace base;
CtpMdSpi::CtpMdSpi(CtpMdApi *api)
    : api_(api) {}

void CtpMdSpi::OnFrontConnected() {
  LOG_INF << "OnFrontConnected.";
  api_->Login();
}

void CtpMdSpi::OnHeartBeatWarning(int nTimeLapse) {
  LOG_WAN << boost::format("OnHeartBeatWarning %1%s.") % nTimeLapse;
}

void CtpMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin,
                              CThostFtdcRspInfoField* pRspInfo,
                              int nRequestID,
                              bool bIsLast) {
  if (pRspInfo && pRspInfo->ErrorID != 0) {
    LOG_ERR << boost::format("Failed to login ctp md : %1%(%2%).") %
               pRspInfo->ErrorID % GB2312ToUtf8(pRspInfo->ErrorMsg);
    return;
  }
  LOG_INF << boost::format("Success to login ctp md %1%") % pRspUserLogin->UserID;
  api_->Subscribe();
}

void CtpMdSpi::OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout,
                               CThostFtdcRspInfoField* pRspInfo,
                               int nRequestID,
                               bool bIsLast) {}

void CtpMdSpi::OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}

void CtpMdSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument,
                                  CThostFtdcRspInfoField* pRspInfo,
                                  int nRequestID,
                                  bool bIsLast) {}

void CtpMdSpi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument,
                                    CThostFtdcRspInfoField* pRspInfo,
                                    int nRequestID,
                                    bool bIsLast) {}

void CtpMdSpi::OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField* pSpecificInstrument,
                                   CThostFtdcRspInfoField* pRspInfo,
                                   int nRequestID,
                                   bool bIsLast) {}

void CtpMdSpi::OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField* pSpecificInstrument,
                                     CThostFtdcRspInfoField* pRspInfo,
                                     int nRequestID,
                                     bool bIsLast) {}

void CtpMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData) {
  if (unlikely(!pDepthMarketData)) {
    LOG_WAN << "OnRtnDepthMarketData : Null parameter";
    return;
  }

  LOG_DBG << boost::format("OnRtnDepthMarketData : TradingDay(%1%), InstrumentID(%2%), "
                           "ExchangeID(%3%), ExchangeInstID(%4%), LastPrice(%5%), "
                           "PreSettlementPrice(%6%), PreClosePrice(%7%), OpenPrice(%8%), "
                           "HighestPrice(%9%), LowestPrice(%10%), Volume(%11%), Turnover(%12%), "
                           "OpenInterest(%13%), ClosePrice(%14%), SettlementPrice(%15%), "
                           "UpperLimitPrice(%16%), LowerLimitPrice(%17%), BidPrice1(%18%), "
                           "BidVolume1(%19%), AskPrice1(%20%), AskVolume1(%21%), ActionDay(%22%)") %
             pDepthMarketData->TradingDay % pDepthMarketData->InstrumentID %
             pDepthMarketData->ExchangeID % pDepthMarketData->ExchangeInstID %
             pDepthMarketData->LastPrice % pDepthMarketData->PreSettlementPrice %
             pDepthMarketData->PreClosePrice % pDepthMarketData->OpenPrice %
             pDepthMarketData->HighestPrice % pDepthMarketData->LowestPrice %
             pDepthMarketData->Volume % pDepthMarketData->Turnover % pDepthMarketData->OpenInterest %
             pDepthMarketData->ClosePrice % pDepthMarketData->SettlementPrice %
             pDepthMarketData->UpperLimitPrice % pDepthMarketData->LowerLimitPrice %
             pDepthMarketData->BidPrice1 % pDepthMarketData->BidVolume1 %
             pDepthMarketData->AskPrice1 % pDepthMarketData->AskVolume1 %
             pDepthMarketData->ActionDay;
  // std::string id = CtpTraderSpi::GetInstrumentId(
  //     pDepthMarketData->InstrumentID, pDepthMarketData->ExchangeID);
  const Instrument *inst = ProductManager::GetInstance()->FindId(pDepthMarketData->InstrumentID);
  if (inst) {
    auto *dm = ClusterManager::GetInstance()->FindDevice(inst->HedgeUnderlying());
    if (dm == nullptr) {
      LOG_ERR << "Can't find device manager for " << inst->HedgeUnderlying();
      return;
    }
    PricePtr price = Message::NewPrice();
    price->header.SetTime();
    price->instrument = inst;
    if (pDepthMarketData->LastPrice < DBL_MAX) {
      price->last.price = pDepthMarketData->LastPrice;
      price->last.volume = 1;
    }
    static bool pub_levels = EnvConfig::GetInstance()->GetBool(EnvVar::PUB_PRICE_LEVELS, true);
    if (pDepthMarketData->BidPrice1 < DBL_MAX) {
      price->bids[0].price = pDepthMarketData->BidPrice1;
      price->bids[0].volume = pDepthMarketData->BidVolume1;
      if (pub_levels && pDepthMarketData->BidPrice2 < DBL_MAX) {
        price->bids[1].price = pDepthMarketData->BidPrice2;
        price->bids[1].volume = pDepthMarketData->BidVolume2;
        if (pDepthMarketData->BidPrice3 < DBL_MAX) {
          price->bids[2].price = pDepthMarketData->BidPrice3;
          price->bids[2].volume = pDepthMarketData->BidVolume3;
          if (pDepthMarketData->BidPrice4 < DBL_MAX) {
            price->bids[3].price = pDepthMarketData->BidPrice4;
            price->bids[3].volume = pDepthMarketData->BidVolume4;
            if (pDepthMarketData->BidPrice5 < DBL_MAX) {
              price->bids[4].price = pDepthMarketData->BidPrice5;
              price->bids[4].volume = pDepthMarketData->BidVolume5;
            }
          }
        }
      }
    }
    if (pDepthMarketData->AskPrice1 < DBL_MAX) {
      price->asks[0].price = pDepthMarketData->AskPrice1;
      price->asks[0].volume = pDepthMarketData->AskVolume1;
      if (pub_levels && pDepthMarketData->AskPrice2 < DBL_MAX) {
        price->asks[1].price = pDepthMarketData->AskPrice2;
        price->asks[1].volume = pDepthMarketData->AskVolume2;
        if (pDepthMarketData->AskPrice3 < DBL_MAX) {
          price->asks[2].price = pDepthMarketData->AskPrice3;
          price->asks[2].volume = pDepthMarketData->AskVolume3;
          if (pDepthMarketData->AskPrice4 < DBL_MAX) {
            price->asks[3].price = pDepthMarketData->AskPrice4;
            price->asks[3].volume = pDepthMarketData->AskVolume4;
            if (pDepthMarketData->AskPrice5 < DBL_MAX) {
              price->asks[4].price = pDepthMarketData->AskPrice5;
              price->asks[4].volume = pDepthMarketData->AskVolume5;
            }
          }
        }
      }
    }
    if (pDepthMarketData->OpenPrice < DBL_MAX) {
      price->open = pDepthMarketData->OpenPrice;
    }
    if (pDepthMarketData->HighestPrice < DBL_MAX) {
      price->high = pDepthMarketData->HighestPrice;
    }
    if (pDepthMarketData->LowestPrice < DBL_MAX) {
      price->low = pDepthMarketData->LowestPrice;
    }
    if (pDepthMarketData->ClosePrice < DBL_MAX) {
      price->close = pDepthMarketData->ClosePrice;
    }
    if (pDepthMarketData->PreClosePrice < DBL_MAX) {
      price->pre_close = pDepthMarketData->PreClosePrice;
    }
    if (pDepthMarketData->PreSettlementPrice < DBL_MAX) {
      price->pre_settlement = pDepthMarketData->PreSettlementPrice;
    }
    if (pDepthMarketData->Turnover < DBL_MAX) {
      price->amount = pDepthMarketData->Turnover;
    }
    if (pDepthMarketData->Volume < DBL_MAX) {
      price->volume = pDepthMarketData->Volume;
    }

    // if (unlikely(inst->Highest() == PRICE_UNDEFINED || inst->Lowest() == PRICE_UNDEFINED))
    // {
    //   Instrument *p = const_cast<Instrument*>(inst);
    //   if (pDepthMarketData->UpperLimitPrice < DBL_MAX)
    //     p->Highest(pDepthMarketData->UpperLimitPrice);
    //   if (pDepthMarketData->LowerLimitPrice < DBL_MAX)
    //     p->Lowest(pDepthMarketData->LowerLimitPrice);
    //   LOG_INF << boost::format("Lowest/Highest price update: %1% - %2%/%3%") %
    //     p->Id() % p->Lowest() % p->Highest();
    //   /// to be done... publish InstrumentReq
    // }
    dm->Publish(price);
  }
}

void CtpMdSpi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField* pForQuoteRsp) {}
