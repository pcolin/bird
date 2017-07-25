#include "CtpTraderSpi.h"
#include "CtpTraderApi.h"
#include "base/common/Likely.h"
#include "base/common/CodeConverter.h"
#include "base/common/CsvReader.h"
#include "base/logger/Logging.h"
#include "model/Future.h"
#include "model/Option.h"
#include "model/ProductManager.h"

#include <boost/format.hpp>

// using namespace base;

CtpTraderSpi::CtpTraderSpi(CtpTraderApi* api) : api_(api)
{
  base::CsvReader reader("INSTRUMENT_CONFIG.csv");
  // auto &lines = reader.GetLines();
  for (auto& line : reader.GetLines())
  {
    config_[line[0]] = { line[1], line[2] };
  }
}

void CtpTraderSpi::OnFrontConnected()
{
  LOG_INF << "OnFrontConnected.";
  api_->Login();
}

void CtpTraderSpi::OnFrontDisconnected(int nReason)
{
  LOG_INF << "OnFrontDisconnected : " << nReason;
}

void CtpTraderSpi::OnHeartBeatWarning(int nTimeLapse)
{
  LOG_WAN << boost::format("OnHeartBeatWarning %1%s.") % nTimeLapse;
}

void CtpTraderSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
  if (pRspInfo && pRspInfo->ErrorID != 0)
  {
    LOG_ERR << boost::format("Failed to login ctp trader : %1%(%2%).") %
      pRspInfo->ErrorID % base::GB2312ToUtf8(pRspInfo->ErrorMsg);
    return;
  }
  LOG_INF << boost::format("Success to login ctp trader %1%") % pRspUserLogin->UserID;
  api_->QueryInstruments();
}

void CtpTraderSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

}

void CtpTraderSpi::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

}

void CtpTraderSpi::OnRspSettlementInfoConfirm(
    CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

}

void CtpTraderSpi::OnRspQrySettlementInfoConfirm(
    CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

}

void CtpTraderSpi::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
  if (pRspInfo && pRspInfo->ErrorID != 0)
  {
    LOG_ERR << boost::format("Failed to query instrument: %1%(%2%).") %
      pRspInfo->ErrorID % base::GB2312ToUtf8(pRspInfo->ErrorMsg);
    return;
  }
  LOG_INF << boost::format("OnRspQryInstrument : InstrumentID(%1%), ExchangeID(%2%), "
      "InstrumentName(%3%), ExchangeInstID(%4%), ProductID(%5%), ProductClass(%6%), "
      "DeliveryYear(%7%), DeliveryMonth(%8%), VolumeMultiple(%9%), PriceTick(%10%), "
      "ExpireDate(%11%), IsTrading(%12%), UnderlyingInstrID(%13%), StrikePrice(%14%), "
      "pRspInfo(%15%), nRequestID(%16%), bIsLast(%17%)") % pInstrument->InstrumentID
    % pInstrument->ExchangeID % pInstrument->InstrumentName % pInstrument->ExchangeInstID
    % pInstrument->ProductID % pInstrument->ProductClass % pInstrument->DeliveryYear
    % pInstrument->DeliveryMonth % pInstrument->VolumeMultiple % pInstrument->PriceTick
    % pInstrument->ExpireDate % pInstrument->IsTrading % pInstrument->UnderlyingInstrID
    % pInstrument->StrikePrice % (pRspInfo != 0) % nRequestID % bIsLast;

  static std::unordered_map<Option*, const std::string> options;
  Exchanges exchange = GetExchange(pInstrument->ExchangeID);
  std::string id = GetInstrumentId(pInstrument->InstrumentID, exchange);
  Instrument* inst = nullptr;
  if (pInstrument->ProductClass == THOST_FTDC_PC_Futures)
  {
    if (config_.find(id) != config_.end()) inst = new Future();
  }
  else if (pInstrument->ProductClass = THOST_FTDC_PC_Options)
  {
    std::string undl = GetInstrumentId(pInstrument->UnderlyingInstrID, exchange);
    if (config_.find(undl) != config_.end())
    {
      Option* op = new Option();
      op->CallPut(pInstrument->OptionsType == THOST_FTDC_CP_CallOptions ? base::Call : base::Put);
      op->Strike(pInstrument->StrikePrice);
      op->SettlementType(base::PhysicalSettlement);
      options.insert(std::make_pair(op, undl));  
      inst = op;
    }
  }

  if (inst)
  {
    inst->Id(id);
    inst->Symbol(pInstrument->InstrumentID);
    inst->Exchange(exchange);
    if (pInstrument->IsTrading == 0) inst->Status(InstrumentStatus::Halt);
    inst->Currency(Currencies::CNY);
    inst->Tick(pInstrument->PriceTick);
    inst->Multiplier(pInstrument->VolumeMultiple);

    if (inst->Type() == InstrumentType::Future)
    {
      ProductManager::GetInstance()->Add(inst);
      LOG_INF << "Add Future " << id;
    }
  }

  if (bIsLast)
  {
    for (auto& it : options)
    {
      const Instrument* undl = ProductManager::GetInstance()->FindId(it.second);
      assert (undl);
      it.first->Underlying(undl);
      const Instrument* hedge_undl =
        ProductManager::GetInstance()->FindId(config_[it.second].hedge_underlying);
      assert(hedge_undl);
      it.first->HedgeUnderlying(hedge_undl);
      ProductManager::GetInstance()->Add(it.first);
      LOG_INF << boost::format("Add Option %1%, Hedge Underlying %2%") %
        it.first->Id() % hedge_undl->Id();
    }
    api_->NotifyInstrumentReady();
  }
}

void CtpTraderSpi::OnRspQryOrder(CThostFtdcOrderField *pOrder,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

}

void CtpTraderSpi::OnRspQryTrade(CThostFtdcTradeField *pTrade,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

}

void CtpTraderSpi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

}

void CtpTraderSpi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

}

/// CTP认为报单非法
void CtpTraderSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

}

/// 交易所认为报单非法
void CtpTraderSpi::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder,
    CThostFtdcRspInfoField *pRspInfo)
{

}

void CtpTraderSpi::OnRtnOrder(CThostFtdcOrderField *pOrder)
{

}

void CtpTraderSpi::OnRtnTrade(CThostFtdcTradeField *pTrade)
{

}

/// CTP认为Quote单非法
void CtpTraderSpi::OnRspQuoteInsert(CThostFtdcInputQuoteField *pInputQuote,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

}

/// 交易所认为Quote单非法
void CtpTraderSpi::OnErrRtnQuoteInsert(CThostFtdcInputQuoteField *pInputQuote,
    CThostFtdcRspInfoField *pRspInfo)
{

}

void CtpTraderSpi::OnRtnQuote(CThostFtdcQuoteField *pQuote)
{

}

/// CTP认为撤单非法
void CtpTraderSpi::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

}

/// 交易所认为撤单非法
void CtpTraderSpi::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction,
    CThostFtdcRspInfoField *pRspInfo)
{

}

/// CTP认为撤Quote单非法
void CtpTraderSpi::OnRspQuoteAction(CThostFtdcInputQuoteActionField *pInputQuoteAction,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

}

/// 交易所认为撤Quote单非法
void CtpTraderSpi::OnErrRtnQuoteAction(CThostFtdcQuoteActionField *pQuoteAction,
    CThostFtdcRspInfoField *pRspInfo)
{

}

void CtpTraderSpi::OnRspBatchOrderAction(CThostFtdcInputBatchOrderActionField *pInputBatchOrderAction,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

}

void CtpTraderSpi::OnErrRtnBatchOrderAction(CThostFtdcBatchOrderActionField *pBatchOrderAction,
    CThostFtdcRspInfoField *pRspInfo)
{

}

void CtpTraderSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

}

/// 合约交易状态通知
void CtpTraderSpi::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus)
{

}

/// CTP认为投资者询价指令非法
void CtpTraderSpi::OnRspForQuoteInsert(CThostFtdcInputForQuoteField *pInputForQuote,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

}

/// 交易所认为投资者询价指令非法，若合法则无信息返回
void CtpTraderSpi::OnErrRtnForQuoteInsert(CThostFtdcInputForQuoteField *pInputForQuote,
    CThostFtdcRspInfoField *pRspInfo)
{

}

/// 做市商接收询价
void CtpTraderSpi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp)
{

}

Exchanges CtpTraderSpi::GetExchange(const char* exchange) const
{
  assert (strlen(exchange) > 2);
  switch (exchange[1])
  {
    case 'C':
    case 'c':
      return Exchanges::DE;
    case 'Z':
    case 'z':
      return Exchanges::ZE;
    case 'F':
    case 'f':
      return Exchanges::CF;
    case 'H':
    case 'h':
      return Exchanges::SF;
    default:
      assert(false);
  }
}

std::string CtpTraderSpi::GetInstrumentId(char* id, Exchanges exchange) const
{
  std::string ret(id);
  switch (exchange)
  {
    case Exchanges::DE:
      return ret += "DE";
    case Exchanges::ZE:
      return ret += "ZE";
    case Exchanges::CF:
      return ret += "CF";
    case Exchanges::SF:
      return ret += "SF";
    default:
      assert(false);
      return "";
  }
}

InstrumentStatus CtpTraderSpi::GetInstrumentStatus(TThostFtdcInstrumentStatusType status) const
{

}
