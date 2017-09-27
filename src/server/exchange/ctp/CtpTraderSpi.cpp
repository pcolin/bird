#include "CtpTraderSpi.h"
#include "CtpTraderApi.h"
#include "base/common/Likely.h"
#include "base/common/CodeConverter.h"
#include "base/common/CsvReader.h"
#include "base/logger/Logging.h"
#include "config/EnvConfig.h"
#include "model/Future.h"
#include "model/Option.h"
#include "model/ProductManager.h"
#include "model/PositionManager.h"
#include "model/OrderManager.h"
#include "model/TradeManager.h"
#include "strategy/DeviceManager.h"
#include "strategy/ClusterManager.h"

#include <boost/format.hpp>

// using namespace base;

CtpTraderSpi::CtpTraderSpi(CtpTraderApi* api) : api_(api)
{
  base::CsvReader reader(EnvConfig::GetInstance()->GetString(EnvVar::CONFIG_FILE));
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
  LOG_INF << boost::format("Success to login ctp trader %1% with MaxOrderRef(%2%), FrontID(%3%)"
      "SessionID(%4%)") % pRspUserLogin->UserID % pRspUserLogin->MaxOrderRef %pRspUserLogin->FrontID %
    pRspUserLogin->SessionID;
  // api_->SetMaxOrderRef(atoi(pRspUserLogin->MaxOrderRef));
  api_->OnUserLogin(atoi(pRspUserLogin->MaxOrderRef), pRspUserLogin->FrontID,
      pRspUserLogin->SessionID);
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
  LOG_INF << boost::format("OnRspQryInstrument: InstrumentID(%1%), ExchangeID(%2%), "
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
  static std::unordered_map<Future*, const std::string> futures;
  Exchanges exchange = GetExchange(pInstrument->ExchangeID);
  // std::string id = GetInstrumentId(pInstrument->InstrumentID, pInstrument->ExchangeID);
  const std::string id = pInstrument->InstrumentID;
  Instrument* inst = nullptr;
  if (pInstrument->ProductClass == THOST_FTDC_PC_Futures)
  {
    auto it = config_.find(id);
    if (it != config_.end())
    {
      Future *future = new Future();
      future->Id(id);
      future->Underlying(future);
      if (id == it->second.hedge_underlying)
      {
        future->HedgeUnderlying(future);
        ProductManager::GetInstance()->Add(future);
        LOG_INF << "Add Future " << id;
      }
      futures.emplace(future, it->second.hedge_underlying);
      inst = future;
    }
  }
  else if (pInstrument->ProductClass = THOST_FTDC_PC_Options)
  {
    // std::string undl = GetInstrumentId(pInstrument->UnderlyingInstrID, pInstrument->ExchangeID);
    std::string undl = pInstrument->UnderlyingInstrID;
    if (config_.find(undl) != config_.end())
    {
      Option* op = new Option();
      op->Id(id);
      op->CallPut(pInstrument->OptionsType == THOST_FTDC_CP_CallOptions ? base::Call : base::Put);
      op->Strike(pInstrument->StrikePrice);
      op->SettlementType(base::PhysicalSettlement);
      options.emplace(op, undl);
      inst = op;
    }
  }

  if (inst)
  {
    inst->Symbol(pInstrument->InstrumentID);
    inst->Exchange(exchange);
    if (pInstrument->IsTrading == 0) inst->Status(InstrumentStatus::Halt);
    inst->Currency(Currencies::CNY);
    inst->Tick(pInstrument->PriceTick);
    inst->Multiplier(pInstrument->VolumeMultiple);

    // if (inst->Type() == InstrumentType::Future)
    // {
    //   ProductManager::GetInstance()->Add(inst);
    //   LOG_INF << "Add Future " << id;
    // }
  }

  if (bIsLast)
  {
    for (auto &it : futures)
    {
      if (it.first->HedgeUnderlying() != nullptr)
      {
        ClusterManager::GetInstance()->AddDevice(it.first);
      }
      else
      {
        LOG_DBG << "Begin to deal with future " << it.second;
        const Instrument* undl = ProductManager::GetInstance()->FindId(it.second);
        assert (undl);
        it.first->HedgeUnderlying(undl);
        ProductManager::GetInstance()->Add(it.first);
        LOG_INF << "Add Future " << it.first->Id();
      }
    }
    for (auto &it : options)
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
    api_->QueryPositions();
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
  if (pRspInfo && pRspInfo->ErrorID != 0)
  {
    LOG_ERR << boost::format("Failed to query position: %1%(%2%).") %
      pRspInfo->ErrorID % base::GB2312ToUtf8(pRspInfo->ErrorMsg);
    return;
  }
  static std::unordered_map<const Instrument*, PositionPtr> positions;
  if (pInvestorPosition)
  {
    LOG_INF << boost::format("OnRspQryInvestorPosition: InstrumentID(%1%), PosiDirection(%2%)"
        "YdPosition(%3%), Position(%4%), LongFrozen(%5%), ShortFrozen(%6%), PositionDate(%7%)") %
      pInvestorPosition->InstrumentID % pInvestorPosition->PosiDirection %
      pInvestorPosition->YdPosition % pInvestorPosition->Position % pInvestorPosition->LongFrozen %
      pInvestorPosition->ShortFrozen % pInvestorPosition->PositionDate;
    auto *inst = ProductManager::GetInstance()->FindSymbol(pInvestorPosition->InstrumentID);
    if (inst)
    {
      PositionPtr pos = positions[inst];
      if (!pos) 
      {
        pos = std::make_shared<PROTO::Position>();
        positions[inst] = pos;
      }
      if (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Long)
      {
        pos->set_total_long(pInvestorPosition->Position);
        pos->set_liquid_long(pInvestorPosition->Position - pInvestorPosition->LongFrozen);
        pos->set_yesterday_long(pInvestorPosition->YdPosition);
      }
      else if (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Short)
      {
        pos->set_total_short(pInvestorPosition->Position);
        pos->set_liquid_short(pInvestorPosition->Position - pInvestorPosition->ShortFrozen);
        pos->set_yesterday_short(pInvestorPosition->YdPosition);
      }
    }
  }

  if (bIsLast)
  {
    for (auto &p : positions)
    {
      ClusterManager::GetInstance()->FindDevice(p.first->HedgeUnderlying())->Publish(p.second);
    }
  }
}

void CtpTraderSpi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

}

/// CTP认为报单非法
void CtpTraderSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
  assert(pInputOrder && pRspInfo);
  const std::string err = base::GB2312ToUtf8(pRspInfo->ErrorMsg);
  LOG_ERR << boost::format("OnRspOrderInsert: InstrumentID(%1%), OrderRef(%2%), ErrorID(%3%), "
      "ErrorMsg(%4%), nRequestID(%5%), bIsLast(%6%)") %
    pInputOrder->InstrumentID % pInputOrder->OrderRef % pRspInfo->ErrorID % err % nRequestID %
    bIsLast;
  auto ord = api_->RemoveOrder(atoi(pInputOrder->OrderRef));
  if (ord)
  {
    auto update = Message::NewOrder(ord);
    update->status = OrderStatus::Rejected;
    update->note = std::move(err);
    api_->OnOrderResponse(update);
  }
  else
  {
    LOG_ERR << "Can't find order " << pInputOrder->OrderRef;
  }
}

/// 交易所认为报单非法
void CtpTraderSpi::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder,
    CThostFtdcRspInfoField *pRspInfo)
{
  assert(pInputOrder && pRspInfo);
  const std::string err = base::GB2312ToUtf8(pRspInfo->ErrorMsg);
  LOG_ERR << boost::format("OnRspOrderInsert: InstrumentID(%1%), OrderRef(%2%), ErrorID(%3%), "
      "ErrorMsg(%4%)") %
    pInputOrder->InstrumentID % pInputOrder->OrderRef % pRspInfo->ErrorID % err;
  auto ord = api_->RemoveOrder(atoi(pInputOrder->OrderRef));
  if (ord)
  {
    auto update = Message::NewOrder(ord);
    update->status = OrderStatus::Rejected;
    update->note = err;
    api_->OnOrderResponse(update);
  }
  else
  {
    LOG_ERR << "Can't find order " << pInputOrder->OrderRef;
  }
}

void CtpTraderSpi::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
  assert(pOrder);
  LOG_INF << boost::format("OnRtnOrder: InstrumentID(%1%), OrderRef(%2%), Direction(%3%), "
      "LimitPrice(%4%), VolumeTotalOriginal(%5%), TimeCondition(%6%), OrderLocalID(%7%), "
      "ExchangeID(%8%), OrderSysID(%9%), OrderStatus(%10%), VolumeTraded(%11%), VolumeTotal(%12%), "
      "StatusMsg(%13%), FrontID(%14%), SessionID(%15%), TraderID(%16%)") %
    pOrder->InstrumentID % pOrder->OrderRef % pOrder->Direction % pOrder->LimitPrice %
    pOrder->VolumeTotalOriginal % pOrder->TimeCondition % pOrder->OrderLocalID % pOrder->ExchangeID %
    pOrder->OrderSysID % pOrder->OrderStatus % pOrder->VolumeTraded % pOrder->VolumeTotal %
    pOrder->StatusMsg % pOrder->FrontID % pOrder->SessionID % pOrder->TraderID;
  if (pOrder->OrderStatus == THOST_FTDC_OST_Unknown)
  {
    auto ord = api_->FindOrder(atoi(pOrder->OrderRef));
    if (ord)
    {
      auto update = Message::NewOrder(ord);
      update->counter_id = pOrder->OrderRef;
      update->status = OrderStatus::Submitted;
      update->header.SetInterval(1);
      api_->OnOrderResponse(update);
    }
    else
    {
      LOG_ERR << "Failed to find order by order ref " << pOrder->OrderRef;
    }
  }
  else if (pOrder->OrderStatus == THOST_FTDC_OST_NoTradeQueueing)
  {
    auto ord = api_->RemoveOrder(atoi(pOrder->OrderRef));
    if (ord)
    {
      auto update = Message::NewOrder(ord);
      update->counter_id = pOrder->OrderRef;
      update->exchange_id = pOrder->OrderSysID;
      update->status = OrderStatus::New;
      update->header.SetInterval(2);
      api_->OnOrderResponse(update);
    }
    else
    {
      LOG_ERR << "Failed to find order by order ref " << pOrder->OrderRef;
    }
  }
  else if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled)
  {
    if (strlen(pOrder->OrderSysID) > 0)
    {
      UpdateOrder(pOrder->OrderSysID, 0, OrderStatus::Canceled);
    }
    else
    {
      auto ord = api_->RemoveOrder(atoi(pOrder->OrderRef));
      if (ord)
      {
        auto update = Message::NewOrder(ord);
        update->counter_id = pOrder->OrderRef;
        update->status = OrderStatus::Rejected;
        update->note = base::GB2312ToUtf8(pOrder->StatusMsg);
        api_->OnOrderResponse(update);
      }
      else
      {
        LOG_ERR << "Failed to find order by order ref " << pOrder->OrderRef;
      }
    }
  }
  else if (pOrder->OrderStatus == THOST_FTDC_OST_AllTraded)
  {
    UpdateOrder(pOrder->OrderSysID, pOrder->VolumeTraded, OrderStatus::Filled);
  }
  else if (pOrder->OrderStatus == THOST_FTDC_OST_PartTradedQueueing)
  {
    UpdateOrder(pOrder->OrderSysID, pOrder->VolumeTraded, OrderStatus::PartialFilled);
  }
  else
  {
    LOG_WAN << "Unexpected order status " << pOrder->OrderStatus;
  }
}

void CtpTraderSpi::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
  assert(pTrade);
  LOG_INF << boost::format("OnRtnTrade: InstrumentID(%1%), OrderRef(%2%), OrderLocalID(%3%) "
      "OrderSysID(%4%), TradeID(%5%), Direction(%6%), Price(%7%), Volume(%8%), TradeDate(%9%), "
      "TradeTime(%10%)") %
    pTrade->InstrumentID % pTrade->OrderRef % pTrade->OrderLocalID % pTrade->OrderSysID %
    pTrade->TradeID % pTrade->Direction % pTrade->Price % pTrade->Volume % pTrade->TradeDate %
    pTrade->TradeTime;
  const Instrument *inst = ProductManager::GetInstance()->FindId(pTrade->InstrumentID);
  if (inst)
  {
    auto trade = Message::NewTrade();
    trade->instrument = inst;
    auto ord = OrderManager::GetInstance()->FindOrder(pTrade->OrderSysID);
    if (ord)
    {
      trade->order_id = ord->id;
      auto update = Message::NewOrder(ord);
      if (update->executed_volume == pTrade->Volume)
      {
        update->avg_executed_price = pTrade->Price;
      }
      else if (update->executed_volume > pTrade->Volume)
      {
        update->avg_executed_price = (update->avg_executed_price * (update->executed_volume -
              pTrade->Volume) + pTrade->Price * pTrade->Volume) / update->executed_volume;
      }
      api_->OnOrderResponse(update);
    }
    else
    {
      LOG_ERR << "Received a trade with unkown exchange order id " << pTrade->OrderSysID;
    }
    trade->price = pTrade->Price;
    trade->volume = pTrade->Volume;
    TradeManager::GetInstance()->OnTrade(trade);
    ClusterManager::GetInstance()->FindDevice(inst->HedgeUnderlying())->Publish(trade);
  }
  else
  {
    LOG_ERR << "Received a trade with unkown instrument " << pTrade->InstrumentID;
  }
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
  assert (pInputOrderAction && pRspInfo);
  LOG_ERR << boost::format("OnRspOrderAction: FrontID(%1%), SessionID(%2%), OrderRef(%3%), "
      "OrderSysID(%4%), InstrumentID(%5%), ErrorID(%6%), ErrorMsg(%7%), nRequestID(%8%), "
      "bIsLast(%9%)") %
    pInputOrderAction->FrontID % pInputOrderAction->SessionID % pInputOrderAction->OrderRef %
    pInputOrderAction->OrderSysID % pInputOrderAction->InstrumentID % pRspInfo->ErrorID %
    base::GB2312ToUtf8(pRspInfo->ErrorMsg) % nRequestID % bIsLast;
}

/// 交易所认为撤单非法
void CtpTraderSpi::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction,
    CThostFtdcRspInfoField *pRspInfo)
{
  assert (pOrderAction && pRspInfo);
  LOG_ERR << boost::format("OnErrRtnOrderAction: FrontID(%1%), SessionID(%2%), OrderRef(%3%), "
      "OrderSysID(%4%), InstrumentID(%5%), ErrorID(%6%), ErrorMsg(%7%)") %
    pOrderAction->FrontID % pOrderAction->SessionID % pOrderAction->OrderRef %
    pOrderAction->OrderSysID % pOrderAction->InstrumentID % pRspInfo->ErrorID %
    base::GB2312ToUtf8(pRspInfo->ErrorMsg);
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

std::string CtpTraderSpi::GetInstrumentId(char* id, char *exchange)
{
  assert (strlen(exchange) > 2);
  std::string ret(id);
  switch (exchange[1])
  {
    case 'C':
    case 'c':
      return ret += "DE";
    case 'Z':
    case 'z':
      return ret += "ZE";
    case 'F':
    case 'f':
      return ret += "CF";
    case 'H':
    case 'h':
      return ret += "SF";
    default:
      assert(false);
  }
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

InstrumentStatus CtpTraderSpi::GetInstrumentStatus(TThostFtdcInstrumentStatusType status) const
{

}

void CtpTraderSpi::UpdateOrder(const char *exchange_id, base::VolumeType volume, OrderStatus status)
{
  auto ord = OrderManager::GetInstance()->FindOrder(exchange_id);
  if (ord)
  {
    auto update = Message::NewOrder(ord);
    update->executed_volume = volume;
    update->status = status;
    api_->OnOrderResponse(update);
  }
  else
  {
    LOG_ERR << "Failed to find order by exchange id " << exchange_id;
  }
}