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
#include "model/ParameterManager.h"
#include "model/PositionManager.h"
#include "model/OrderManager.h"
#include "model/TradeManager.h"
#include "model/Middleware.h"
#include "strategy/DeviceManager.h"
#include "strategy/ClusterManager.h"
#include "boost/format.hpp"

CtpTraderSpi::CtpTraderSpi(CtpTraderApi* api)
    : api_(api) {
  base::CsvReader reader(EnvConfig::GetInstance()->GetString(EnvVar::CONFIG_FILE));
  for (auto& line : reader.GetLines()) {
    config_[line[0]] = { line[1], line[2] };
  }
}

void CtpTraderSpi::OnFrontConnected() {
  LOG_INF << "OnFrontConnected.";
  if (login == false) {
    api_->Login();
    login = true;
  }
}

void CtpTraderSpi::OnFrontDisconnected(int nReason) {
  LOG_INF << "OnFrontDisconnected : " << nReason;
  // login = false;
}

void CtpTraderSpi::OnHeartBeatWarning(int nTimeLapse) {
  LOG_WAN << boost::format("OnHeartBeatWarning %1%s.") % nTimeLapse;
}

void CtpTraderSpi::OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin,
                                  CThostFtdcRspInfoField* pRspInfo,
                                  int nRequestID,
                                  bool bIsLast) {
  if (pRspInfo && pRspInfo->ErrorID != 0) {
    LOG_ERR << boost::format("Failed to login ctp trader : %1%(%2%).") %
               pRspInfo->ErrorID % base::GB2312ToUtf8(pRspInfo->ErrorMsg);
    login = false;
    return;
  }
  LOG_INF << boost::format("Success to login ctp trader %1% with MaxOrderRef(%2%), FrontID(%3%)"
                           "SessionID(%4%)") %
             pRspUserLogin->UserID % pRspUserLogin->MaxOrderRef %pRspUserLogin->FrontID %
             pRspUserLogin->SessionID;
  // api_->SetMaxOrderRef(atoi(pRspUserLogin->MaxOrderRef));
  api_->OnUserLogin(atoi(pRspUserLogin->MaxOrderRef), pRspUserLogin->FrontID,
                    pRspUserLogin->SessionID);
  api_->QueryInstruments();
}

void CtpTraderSpi::OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout,
                                   CThostFtdcRspInfoField* pRspInfo,
                                   int nRequestID,
                                   bool bIsLast) {
}

void CtpTraderSpi::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField* pSettlementInfo,
                                          CThostFtdcRspInfoField* pRspInfo,
                                          int nRequestID,
                                          bool bIsLast) {
}

void CtpTraderSpi::OnRspSettlementInfoConfirm(
    CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm,
    CThostFtdcRspInfoField* pRspInfo,
    int nRequestID,
    bool bIsLast) {
}

void CtpTraderSpi::OnRspQrySettlementInfoConfirm(
    CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm,
    CThostFtdcRspInfoField* pRspInfo,
    int nRequestID,
    bool bIsLast) {
}

void CtpTraderSpi::OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument,
                                      CThostFtdcRspInfoField* pRspInfo,
                                      int nRequestID,
                                      bool bIsLast) {
  if (pRspInfo && pRspInfo->ErrorID != 0) {
    LOG_ERR << boost::format("Failed to query instrument: %1%(%2%).") %
               pRspInfo->ErrorID % base::GB2312ToUtf8(pRspInfo->ErrorMsg);
    return;
  }
  LOG_INF << boost::format("OnRspQryInstrument: InstrumentID(%1%), ExchangeID(%2%), "
                           "InstrumentName(%3%), ExchangeInstID(%4%), ProductID(%5%), "
                           "ProductClass(%6%), DeliveryYear(%7%), DeliveryMonth(%8%), "
                           "VolumeMultiple(%9%), PriceTick(%10%), ExpireDate(%11%), "
                           "IsTrading(%12%), UnderlyingInstrID(%13%), StrikePrice(%14%), "
                           "pRspInfo(%15%), nRequestID(%16%), bIsLast(%17%)") %
             pInstrument->InstrumentID % pInstrument->ExchangeID % pInstrument->InstrumentName %
             pInstrument->ExchangeInstID % pInstrument->ProductID % pInstrument->ProductClass %
             pInstrument->DeliveryYear % pInstrument->DeliveryMonth % pInstrument->VolumeMultiple %
             pInstrument->PriceTick % pInstrument->ExpireDate % pInstrument->IsTrading %
             pInstrument->UnderlyingInstrID % pInstrument->StrikePrice % (pRspInfo != 0) %
             nRequestID % bIsLast;

  static std::unordered_map<Option*, const std::string> options;
  static std::unordered_map<Future*, const std::string> futures;
  Proto::Exchange exchange = GetExchange(pInstrument->ExchangeID);
  // std::string id = GetInstrumentId(pInstrument->InstrumentID, pInstrument->ExchangeID);
  const std::string id = pInstrument->InstrumentID;
  Instrument* inst = nullptr;
  if (pInstrument->ProductClass == THOST_FTDC_PC_Futures) {
    auto it = config_.find(id);
    if (it != config_.end()) {
      Future *future = new Future();
      future->Id(id);
      future->Underlying(future);
      future->Maturity(boost::gregorian::from_undelimited_string(pInstrument->ExpireDate)); /// DCE
      if (id == it->second.hedge_underlying) {
        future->HedgeUnderlying(future);
        ProductManager::GetInstance()->Add(future);
        LOG_INF << "Add Future " << id;
      }
      futures.emplace(future, it->second.hedge_underlying);
      inst = future;
    }
  } else if (pInstrument->ProductClass = THOST_FTDC_PC_Options) {
    // std::string undl = GetInstrumentId(pInstrument->UnderlyingInstrID, pInstrument->ExchangeID);
    std::string undl = pInstrument->UnderlyingInstrID;
    if (config_.find(undl) != config_.end()) {
      Option* op = new Option();
      op->Id(id);
      op->CallPut(pInstrument->OptionsType == THOST_FTDC_CP_CallOptions ? Proto::Call : Proto::Put);
      op->Strike(pInstrument->StrikePrice);
      op->SettlementType(Proto::PhysicalSettlement);
      op->ExerciseType(Proto::American);
      op->Maturity(boost::gregorian::from_undelimited_string(pInstrument->ExpireDate)); /// DCE
      options.emplace(op, undl);
      inst = op;
    }
  }

  if (inst) {
    inst->Symbol(pInstrument->InstrumentID);
    inst->Product(pInstrument->ProductID);
    inst->Exchange(exchange);
    if (pInstrument->IsTrading) {
      auto exch = ParameterManager::GetInstance()->GetExchange();
      if (exch) {
        if (exch->IsTradingTime(inst->Maturity())) {
          inst->Status(Proto::InstrumentStatus::Trading);
        } else {
          inst->Status(Proto::InstrumentStatus::PreOpen);
        }
      } else {
        LOG_ERR << "Failed to get exchange parameter.";
      }
    } else {
      inst->Status(Proto::InstrumentStatus::Halt);
    }
    LOG_DBG << boost::format("Set %1% status: %2%") % inst->Id() %
               Proto::InstrumentStatus_Name(inst->Status());
    inst->Currency(Proto::Currency::CNY);
    inst->Tick(pInstrument->PriceTick);
    inst->Multiplier(pInstrument->VolumeMultiple);
  }

  if (bIsLast) {
    // auto req = Message::NewProto<Proto::InstrumentReq>();
    for (auto &it : futures) {
      if (it.first->HedgeUnderlying() != nullptr) {
        ClusterManager::GetInstance()->AddDevice(it.first);
      } else {
        LOG_DBG << "Begin to deal with future " << it.second;
        const Instrument* undl = ProductManager::GetInstance()->FindId(it.second);
        assert (undl);
        it.first->HedgeUnderlying(undl);
        ProductManager::GetInstance()->Add(it.first);
        LOG_INF << "Add Future " << it.first->Id();
      }
      // it.first->Serialize(req->add_instruments());
    }
    for (auto &it : options) {
      const Instrument* undl = ProductManager::GetInstance()->FindId(it.second);
      assert (undl);
      it.first->Underlying(undl);
      const Instrument* hedge_undl = ProductManager::GetInstance()->FindId(
                                     config_[it.second].hedge_underlying);
      assert(hedge_undl);
      it.first->HedgeUnderlying(hedge_undl);
      ProductManager::GetInstance()->Add(it.first);
      LOG_INF << boost::format("Add Option %1%, Hedge Underlying %2%") %
                 it.first->Id() % hedge_undl->Id();
      // it.first->Serialize(req->add_instruments());
    }
    // req->set_exchange(exchange);
    // req->set_type(Proto::RequestType::Set);
    // Middleware::GetInstance()->Publish(req);
    // api_->NotifyInstrumentReady();
    // api_->QueryPositions();
    api_->QueryFutureCommissionRate();
  }
}

void CtpTraderSpi::OnRspQryInstrumentCommissionRate(
    CThostFtdcInstrumentCommissionRateField* pInstrumentCommissionRate,
    CThostFtdcRspInfoField* pRspInfo,
    int nRequestID,
    bool bIsLast) {
  if (pRspInfo && pRspInfo->ErrorID != 0) {
    LOG_ERR << boost::format("Failed to query future commission rate: %1%(%2%).") %
               pRspInfo->ErrorID % base::GB2312ToUtf8(pRspInfo->ErrorMsg);
    return;
  }
  LOG_INF << boost::format("OnRspQryInstrumentCommissionRate: InstrumentID(%1%), BrokerID(%2%), "
                           "InvestorID(%3%), OpenRatioByMoney(%4%), OpenRatioByVolume(%5%), "
                           "CloseRatioByMoney(%6%), CloseRatioByVolume(%7%), "
                           "CloseTodayRatioByMoney(%8%), CloseTodayRatioByVolume(%9%)") %
             pInstrumentCommissionRate->InstrumentID % pInstrumentCommissionRate->BrokerID %
             pInstrumentCommissionRate->InvestorID % pInstrumentCommissionRate->OpenRatioByMoney %
             pInstrumentCommissionRate->OpenRatioByVolume %
             pInstrumentCommissionRate->CloseRatioByMoney %
             pInstrumentCommissionRate->CloseRatioByVolume %
             pInstrumentCommissionRate->CloseTodayRatioByMoney %
             pInstrumentCommissionRate->CloseTodayRatioByVolume;

  /// to be done...
  auto instruments = ProductManager::GetInstance()->FindInstruments(
      [&](const Instrument *inst) {
        return inst->Product() == pInstrumentCommissionRate->InstrumentID;
      });
  if (instruments.size() > 0) {
    if (pInstrumentCommissionRate->OpenRatioByMoney > 0 ||
        pInstrumentCommissionRate->CloseRatioByMoney > 0 ||
        pInstrumentCommissionRate->CloseTodayRatioByMoney > 0) {
      for(auto *instrument : instruments) {
        Instrument *inst = const_cast<Instrument*>(instrument);
        inst->CommissionType(Proto::CommissionType::Money);
        inst->OpenCommission(pInstrumentCommissionRate->OpenRatioByMoney);
        inst->CloseCommission(pInstrumentCommissionRate->CloseRatioByMoney);
        inst->CloseTodayCommission(pInstrumentCommissionRate->CloseTodayRatioByMoney);
      }
    } else {
      for(auto *instrument : instruments) {
        Instrument *inst = const_cast<Instrument*>(instrument);
        inst->CommissionType(Proto::CommissionType::Volume);
        inst->OpenCommission(pInstrumentCommissionRate->OpenRatioByVolume);
        inst->CloseCommission(pInstrumentCommissionRate->CloseRatioByVolume);
        inst->CloseTodayCommission(pInstrumentCommissionRate->CloseTodayRatioByVolume);
      }
    }
  }

  if (bIsLast) {
    api_->QueryOptionCommissionRate();
  }
}

void CtpTraderSpi::OnRspQryOptionInstrCommRate(
    CThostFtdcOptionInstrCommRateField* pOptionInstrCommRate,
    CThostFtdcRspInfoField* pRspInfo,
    int nRequestID,
    bool bIsLast) {
  if (pRspInfo && pRspInfo->ErrorID != 0) {
    LOG_ERR << boost::format("Failed to query option commission rate: %1%(%2%).") %
               pRspInfo->ErrorID % base::GB2312ToUtf8(pRspInfo->ErrorMsg);
    return;
  }

  LOG_INF << boost::format("OnRspQryOptionInstrCommRate: InstrumentID(%1%), BrokerID(%2%), "
                           "InvestorID(%3%), OpenRatioByMoney(%4%), OpenRatioByVolume(%5%), "
                           "CloseRatioByMoney(%6%), CloseRatioByVolume(%7%), "
                           "CloseTodayRatioByMoney(%8%), CloseTodayRatioByVolume(%9%)") %
             pOptionInstrCommRate->InstrumentID % pOptionInstrCommRate->BrokerID %
             pOptionInstrCommRate->InvestorID % pOptionInstrCommRate->OpenRatioByMoney %
             pOptionInstrCommRate->OpenRatioByVolume % pOptionInstrCommRate->CloseRatioByMoney %
             pOptionInstrCommRate->CloseRatioByVolume %
             pOptionInstrCommRate->CloseTodayRatioByMoney %
             pOptionInstrCommRate->CloseTodayRatioByVolume;

  /// to be done...
  auto instruments = ProductManager::GetInstance()->FindInstruments(
      [&](const Instrument *inst) {
        return inst->Product() == pOptionInstrCommRate->InstrumentID;
      });
  if (instruments.size() > 0) {
    if (pOptionInstrCommRate->OpenRatioByMoney > 0 || pOptionInstrCommRate->CloseRatioByMoney > 0 ||
        pOptionInstrCommRate->CloseTodayRatioByMoney > 0) {
      for(auto *instrument : instruments) {
        Instrument *inst = const_cast<Instrument*>(instrument);
        inst->CommissionType(Proto::CommissionType::Money);
        inst->OpenCommission(pOptionInstrCommRate->OpenRatioByMoney);
        inst->CloseCommission(pOptionInstrCommRate->CloseRatioByMoney);
        inst->CloseTodayCommission(pOptionInstrCommRate->CloseTodayRatioByMoney);
      }
    } else {
      for(auto *instrument : instruments) {
        Instrument *inst = const_cast<Instrument*>(instrument);
        inst->CommissionType(Proto::CommissionType::Volume);
        inst->OpenCommission(pOptionInstrCommRate->OpenRatioByVolume);
        inst->CloseCommission(pOptionInstrCommRate->CloseRatioByVolume);
        inst->CloseTodayCommission(pOptionInstrCommRate->CloseTodayRatioByVolume);
      }
    }
  }

  if (bIsLast) {
    // api_->QueryMMOptionCommissionRate();
    api_->QueryMarketData();
  }
}

void CtpTraderSpi::OnRspQryMMOptionInstrCommRate(
    CThostFtdcMMOptionInstrCommRateField* pMMOptionInstrCommRate,
    CThostFtdcRspInfoField* pRspInfo,
    int nRequestID,
    bool bIsLast) {
  if (pRspInfo && pRspInfo->ErrorID != 0) {
    LOG_ERR << boost::format("Failed to query market maker option commission rate: %1%(%2%).") %
               pRspInfo->ErrorID % base::GB2312ToUtf8(pRspInfo->ErrorMsg);
    return;
  }

  if (pMMOptionInstrCommRate) { /// is null in simulation enrionment
    LOG_INF << boost::format("OnRspMMQryOptionInstrCommRate: InstrumentID(%1%), BrokerID(%2%), "
                             "InvestorID(%3%), OpenRatioByMoney(%4%), OpenRatioByVolume(%5%), "
                             "CloseRatioByMoney(%6%), CloseRatioByVolume(%7%), "
                             "CloseTodayRatioByMoney(%8%), CloseTodayRatioByVolume(%9%)") %
               "" % "" %
               // pMMOptionInstrCommRate->InstrumentID % pMMOptionInstrCommRate->BrokerID %
               "" % pMMOptionInstrCommRate->OpenRatioByMoney %
               // pMMOptionInstrCommRate->InvestorID % pMMOptionInstrCommRate->OpenRatioByMoney %
               pMMOptionInstrCommRate->OpenRatioByVolume %
               pMMOptionInstrCommRate->CloseRatioByMoney %
               pMMOptionInstrCommRate->CloseRatioByVolume %
               pMMOptionInstrCommRate->CloseTodayRatioByMoney %
               pMMOptionInstrCommRate->CloseTodayRatioByVolume;
  }

  /// to be done...

  if (bIsLast) {
    api_->QueryMarketData();
  }
}

void CtpTraderSpi::OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData,
                                           CThostFtdcRspInfoField* pRspInfo,
                                           int nRequestID,
                                           bool bIsLast) {
  if (pRspInfo && pRspInfo->ErrorID != 0) {
    LOG_ERR << boost::format("Failed to query market data: %1%(%2%).") %
               pRspInfo->ErrorID % base::GB2312ToUtf8(pRspInfo->ErrorMsg);
    return;
  }
  LOG_INF << boost::format("OnRspQryDepthMarketData: InstrumentID(%1%), UpperLimitPrice(%2%), "
                           "LowerLimitPrice(%3%), PreSettlementPrice(%4%)") %
             pDepthMarketData->InstrumentID % pDepthMarketData->UpperLimitPrice %
             pDepthMarketData->LowerLimitPrice % pDepthMarketData->PreSettlementPrice;
  auto *inst = const_cast<Instrument*>(
               ProductManager::GetInstance()->FindId(pDepthMarketData->InstrumentID));
  if (inst) {
    inst->Highest(pDepthMarketData->UpperLimitPrice);
    inst->Lowest(pDepthMarketData->LowerLimitPrice);
    LOG_INF << boost::format("Lowest/Highest price update: %1% - %2%/%3%") %
               inst->Id() % inst->Lowest() % inst->Highest();
  }
  if (bIsLast) {
    auto &instruments = ProductManager::GetInstance()->FindInstruments(
                        [](const Instrument*) { return true; });
    if (instruments.size() > 0) {
      auto req = Message::NewProto<Proto::InstrumentReq>();
      req->set_type(Proto::RequestType::Set);
      req->set_exchange(instruments[0]->Exchange());
      for (const Instrument *inst : instruments) {
        if (inst->Type() == Proto::InstrumentType::Option) {
          auto *op = base::down_cast<const Option*>(inst);
          op->Serialize(req->add_instruments());
        } else {
          inst->Serialize(req->add_instruments());
        }
      }
      Middleware::GetInstance()->Publish(req);
    }
    api_->NotifyInstrumentReady();
    api_->QueryPositions();
  }
}

void CtpTraderSpi::OnRspQryOrder(CThostFtdcOrderField* pOrder,
                                 CThostFtdcRspInfoField* pRspInfo,
                                 int nRequestID,
                                 bool bIsLast) {
}

void CtpTraderSpi::OnRspQryTrade(CThostFtdcTradeField* pTrade,
                                 CThostFtdcRspInfoField* pRspInfo,
                                 int nRequestID,
                                 bool bIsLast) {
}

void CtpTraderSpi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pInvestorPosition,
                                            CThostFtdcRspInfoField* pRspInfo,
                                            int nRequestID,
                                            bool bIsLast) {
  if (pRspInfo && pRspInfo->ErrorID != 0) {
    LOG_ERR << boost::format("Failed to query position: %1%(%2%).") %
               pRspInfo->ErrorID % base::GB2312ToUtf8(pRspInfo->ErrorMsg);
    return;
  }
  static std::unordered_map<const Instrument*, PositionPtr> positions;
  if (pInvestorPosition) {
    LOG_INF << boost::format("OnRspQryInvestorPosition: InstrumentID(%1%), PosiDirection(%2%), "
                             "YdPosition(%3%), Position(%4%), LongFrozen(%5%), ShortFrozen(%6%), "
                             "PositionDate(%7%), OpenVolume(%8%), CloseVolume(%9%), "
                             "TodayPosition(%10%)") %
               pInvestorPosition->InstrumentID % pInvestorPosition->PosiDirection %
               pInvestorPosition->YdPosition % pInvestorPosition->Position %
               pInvestorPosition->LongFrozen % pInvestorPosition->ShortFrozen %
               pInvestorPosition->PositionDate % pInvestorPosition->OpenVolume %
               pInvestorPosition->CloseVolume % pInvestorPosition->TodayPosition;
    auto *inst = ProductManager::GetInstance()->FindId(pInvestorPosition->InstrumentID);
    if (inst) {
      PositionPtr pos = nullptr;
      auto it = positions.find(inst);
      if (it == positions.end()) {
        pos = Message::NewProto<Proto::Position>();
        pos->set_instrument(inst->Id());
        positions[inst] = pos;
      } else {
        pos = it->second;
      }
      if (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Long) {
        pos->set_total_long(pInvestorPosition->Position);
        pos->set_liquid_long(pInvestorPosition->Position - pInvestorPosition->ShortFrozen);
        pos->set_yesterday_long(pInvestorPosition->YdPosition);
      } else if (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Short) {
        pos->set_total_short(pInvestorPosition->Position);
        pos->set_liquid_short(pInvestorPosition->Position - pInvestorPosition->LongFrozen);
        pos->set_yesterday_short(pInvestorPosition->YdPosition);
      }
      // pos->set_time(base::Now());
    }
  }

  if (bIsLast) {
    for (auto &p : positions) {
      ClusterManager::GetInstance()->FindDevice(p.first->HedgeUnderlying())->Publish(p.second);
    }
  }
}

void CtpTraderSpi::OnRspQryTradingAccount(CThostFtdcTradingAccountField* pTradingAccount,
                                          CThostFtdcRspInfoField* pRspInfo,
                                          int nRequestID,
                                          bool bIsLast) {
  if (pRspInfo && pRspInfo->ErrorID != 0) {
    LOG_ERR << boost::format("Failed to query cash: %1%(%2%).") %
               pRspInfo->ErrorID % base::GB2312ToUtf8(pRspInfo->ErrorMsg);
    return;
  }
  if (pTradingAccount) {
    LOG_INF << boost::format("OnRspQryTradingAccount: AccountID(%1%), FrozenMargin(%2%), "
                             "CurrMargin(%3%), FrozenCash(%4%), Available(%5%)") %
               pTradingAccount->AccountID % pTradingAccount->FrozenMargin %
               pTradingAccount->CurrMargin % pTradingAccount->FrozenCash %
               pTradingAccount->Available;
    auto cash = std::make_shared<Proto::Cash>();
    cash->set_currency(Proto::CNY);
    cash->set_account(pTradingAccount->AccountID);
    cash->set_total(pTradingAccount->Available + pTradingAccount->FrozenCash);
    cash->set_available(pTradingAccount->Available);
    cash->set_margin(pTradingAccount->CurrMargin);
    static const double limit = EnvConfig::GetInstance()
                                ->GetDouble(EnvVar::OPTION_CASH_LIMIT);
    cash->set_is_enough(pTradingAccount->Available >= limit);
    ClusterManager::GetInstance()->OnCash(cash);
  }
}

/// CTP认为报单非法
void CtpTraderSpi::OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder,
                                    CThostFtdcRspInfoField* pRspInfo,
                                    int nRequestID,
                                    bool bIsLast) {
  assert(pInputOrder && pRspInfo);
  const std::string err = base::GB2312ToUtf8(pRspInfo->ErrorMsg);
  LOG_ERR << boost::format("OnRspOrderInsert: InstrumentID(%1%), OrderRef(%2%), ErrorID(%3%), "
                           "ErrorMsg(%4%), nRequestID(%5%), bIsLast(%6%)") %
             pInputOrder->InstrumentID % pInputOrder->OrderRef % pRspInfo->ErrorID % err %
             nRequestID % bIsLast;
  auto ord = api_->RemoveOrder(pInputOrder->OrderRef);
  if (ord) {
    auto update = Message::NewOrder(ord);
    update->status = Proto::OrderStatus::Rejected;
    update->note = std::move(err);
    api_->OnOrderResponse(update);
  } else {
    LOG_ERR << "Can't find order " << pInputOrder->OrderRef;
  }
}

/// 交易所认为报单非法
void CtpTraderSpi::OnErrRtnOrderInsert(CThostFtdcInputOrderField* pInputOrder,
                                       CThostFtdcRspInfoField* pRspInfo) {
  assert(pInputOrder && pRspInfo);
  const std::string err = base::GB2312ToUtf8(pRspInfo->ErrorMsg);
  LOG_ERR << boost::format("OnRspOrderInsert: InstrumentID(%1%), OrderRef(%2%), ErrorID(%3%), "
                           "ErrorMsg(%4%)") %
             pInputOrder->InstrumentID % pInputOrder->OrderRef % pRspInfo->ErrorID % err;
  auto ord = api_->RemoveOrder(pInputOrder->OrderRef);
  if (ord) {
    auto update = Message::NewOrder(ord);
    update->status = Proto::OrderStatus::Rejected;
    update->note = err;
    api_->OnOrderResponse(update);
  } else {
    LOG_ERR << "Can't find order " << pInputOrder->OrderRef;
  }
}

void CtpTraderSpi::OnRtnOrder(CThostFtdcOrderField* pOrder) {
  assert(pOrder);
  LOG_INF << boost::format("OnRtnOrder: InstrumentID(%1%), OrderRef(%2%), Direction(%3%), "
                           "LimitPrice(%4%), VolumeTotalOriginal(%5%), TimeCondition(%6%), "
                           "OrderLocalID(%7%), ExchangeID(%8%), OrderSysID(%9%), OrderStatus(%10%), "
                           "VolumeTraded(%11%), VolumeTotal(%12%), StatusMsg(%13%), FrontID(%14%), "
                           "SessionID(%15%), TraderID(%16%)") %
             pOrder->InstrumentID % pOrder->OrderRef % pOrder->Direction % pOrder->LimitPrice %
             pOrder->VolumeTotalOriginal % pOrder->TimeCondition % pOrder->OrderLocalID %
             pOrder->ExchangeID % pOrder->OrderSysID % pOrder->OrderStatus % pOrder->VolumeTraded %
             pOrder->VolumeTotal % pOrder->StatusMsg % pOrder->FrontID % pOrder->SessionID %
             pOrder->TraderID;
  if (pOrder->OrderStatus == THOST_FTDC_OST_Unknown) {
    auto ord = api_->FindAndUpdate(pOrder->OrderRef);
    if (ord) {
      api_->OnOrderResponse(Message::NewOrder(ord));
    }
  } else if (pOrder->OrderStatus == THOST_FTDC_OST_NoTradeQueueing) {
    auto ord = api_->FindAndUpdate(pOrder->OrderRef, pOrder->OrderSysID);
    if (ord) {
      api_->OnOrderResponse(Message::NewOrder(ord));
    }
  } else if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled) {
    if (strlen(pOrder->OrderSysID) > 0) {
      auto ord = api_->RemoveOrder(pOrder->OrderRef);
      if (ord) {
        auto update = Message::NewOrder(ord);
        update->executed_volume = pOrder->VolumeTraded;
        update->status = pOrder->VolumeTraded ? Proto::OrderStatus::PartialFilledCanceled :
                                                Proto::OrderStatus::Canceled;
        api_->OnOrderResponse(update);
      }
    } else {
      auto ord = api_->RemoveOrder(pOrder->OrderRef);
      if (ord) {
        auto update = Message::NewOrder(ord);
        update->counter_id = pOrder->OrderRef;
        update->status = Proto::OrderStatus::Rejected;
        update->note = base::GB2312ToUtf8(pOrder->StatusMsg);
        api_->OnOrderResponse(update);
      } else {
        LOG_ERR << "Failed to find order by order ref " << pOrder->OrderRef;
      }
    }
  }
  else if (pOrder->OrderStatus == THOST_FTDC_OST_AllTraded) {
    auto ord = api_->FindAndUpdate(pOrder->OrderRef, pOrder->VolumeTraded);
    if (ord) {
      api_->OnOrderResponse(Message::NewOrder(ord));
      // auto update = Message::NewOrder(ord);
      // update->executed_volume = pOrder->VolumeTraded;
      // update->status = Proto::OrderStatus::Filled;
      // api_->OnOrderResponse(update);
    }
  } else if (pOrder->OrderStatus == THOST_FTDC_OST_PartTradedQueueing) {
    auto ord = api_->FindAndUpdate(pOrder->OrderRef, pOrder->VolumeTraded);
    if (ord) {
      api_->OnOrderResponse(Message::NewOrder(ord));
    }
  } else {
    LOG_WAN << "Unexpected order status " << pOrder->OrderStatus;
  }
}

void CtpTraderSpi::OnRtnTrade(CThostFtdcTradeField* pTrade) {
  assert(pTrade);
  LOG_INF << boost::format("OnRtnTrade: InstrumentID(%1%), OrderRef(%2%), OrderLocalID(%3%) "
                           "OrderSysID(%4%), TradeID(%5%), Direction(%6%), Price(%7%), Volume(%8%), "
                           "TradeDate(%9%), TradeTime(%10%)") %
             pTrade->InstrumentID % pTrade->OrderRef % pTrade->OrderLocalID % pTrade->OrderSysID %
             pTrade->TradeID % pTrade->Direction % pTrade->Price % pTrade->Volume %
             pTrade->TradeDate % pTrade->TradeTime;
  const Instrument *inst = ProductManager::GetInstance()->FindId(pTrade->InstrumentID);
  if (inst) {
    auto ord = api_->FindAndRemove(pTrade->OrderRef);
    // if (ord == nullptr)
    // {
    //   ord = OrderManager::GetInstance()->FindOrder(pTrade->OrderSysID);
    // }
    if (ord) {
      auto trade = Message::NewTrade();
      trade->header.SetTime();
      trade->instrument = inst;
      trade->order_id = ord->id;
      // auto update = Message::NewOrder(ord);
      // if (update->executed_volume == pTrade->Volume)
      // {
      //   update->avg_executed_price = pTrade->Price;
      // }
      // else if (update->executed_volume > pTrade->Volume)
      // {
      //   update->avg_executed_price = (update->avg_executed_price * (update->executed_volume -
      //         pTrade->Volume) + pTrade->Price * pTrade->Volume) / update->executed_volume;
      // }
      trade->side = ord->side;
      trade->id = (boost::format("%1%%2%") % (ord->IsBid() ? 'B' : 'A') % pTrade->TradeID).str();
      trade->price = pTrade->Price;
      trade->volume = pTrade->Volume;
      trade->time = base::StringToTime(pTrade->TradeDate, pTrade->TradeTime);
      TradeManager::GetInstance()->OnTrade(trade);
      ClusterManager::GetInstance()->FindDevice(inst->HedgeUnderlying())->Publish(trade);
      int date = atoi(pTrade->TradeDate);
      int h = atoi(pTrade->TradeTime);
      int m = atoi(pTrade->TradeTime + 3);
      int s = atoi(pTrade->TradeTime + 6);
      LOG_INF << "Date: " << date << " year: " << (date / 10000 - 1900) << " h: "
        << h << " m: " << m << " s: " << s;
      // api_->OnOrderResponse(update);
    } else {
      LOG_ERR << "Received a trade with unkown exchange order id " << pTrade->OrderSysID;
    }
  } else {
    LOG_ERR << "Received a trade with unkown instrument " << pTrade->InstrumentID;
  }
}

/// CTP认为Quote单非法
void CtpTraderSpi::OnRspQuoteInsert(CThostFtdcInputQuoteField* pInputQuote,
                                    CThostFtdcRspInfoField* pRspInfo,
                                    int nRequestID,
                                    bool bIsLast) {
}

/// 交易所认为Quote单非法
void CtpTraderSpi::OnErrRtnQuoteInsert(CThostFtdcInputQuoteField* pInputQuote,
                                       CThostFtdcRspInfoField* pRspInfo) {
}

void CtpTraderSpi::OnRtnQuote(CThostFtdcQuoteField* pQuote) {
}

/// CTP认为撤单非法
void CtpTraderSpi::OnRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction,
                                    CThostFtdcRspInfoField* pRspInfo,
                                    int nRequestID,
                                    bool bIsLast) {
  assert (pInputOrderAction && pRspInfo);
  LOG_ERR << boost::format("OnRspOrderAction: FrontID(%1%), SessionID(%2%), ExchangeID(%3%), "
                           "OrderRef(%4%), OrderSysID(%5%), InstrumentID(%6%), ErrorID(%7%), "
                           "ErrorMsg(%8%), nRequestID(%9%), bIsLast(%10%)") %
             pInputOrderAction->FrontID % pInputOrderAction->SessionID %
             pInputOrderAction->ExchangeID % pInputOrderAction->OrderRef %
             pInputOrderAction->OrderSysID % pInputOrderAction->InstrumentID %
             pRspInfo->ErrorID % base::GB2312ToUtf8(pRspInfo->ErrorMsg) % nRequestID % bIsLast;
}

/// 交易所认为撤单非法
void CtpTraderSpi::OnErrRtnOrderAction(CThostFtdcOrderActionField* pOrderAction,
                                       CThostFtdcRspInfoField* pRspInfo) {
  assert (pOrderAction && pRspInfo);
  LOG_ERR << boost::format("OnErrRtnOrderAction: FrontID(%1%), SessionID(%2%), ExchangeID(%3%), "
                           "OrderRef(%4%), OrderSysID(%5%), InstrumentID(%6%), ErrorID(%7%), "
                           "ErrorMsg(%8%)") %
             pOrderAction->FrontID % pOrderAction->SessionID % pOrderAction->ExchangeID %
             pOrderAction->OrderRef % pOrderAction->OrderSysID % pOrderAction->InstrumentID %
             pRspInfo->ErrorID % base::GB2312ToUtf8(pRspInfo->ErrorMsg);
}

/// CTP认为撤Quote单非法
void CtpTraderSpi::OnRspQuoteAction(CThostFtdcInputQuoteActionField* pInputQuoteAction,
                                    CThostFtdcRspInfoField* pRspInfo,
                                    int nRequestID,
                                    bool bIsLast) {
}

/// 交易所认为撤Quote单非法
void CtpTraderSpi::OnErrRtnQuoteAction(CThostFtdcQuoteActionField* pQuoteAction,
                                       CThostFtdcRspInfoField* pRspInfo) {
}

void CtpTraderSpi::OnRspBatchOrderAction(
    CThostFtdcInputBatchOrderActionField* pInputBatchOrderAction,
    CThostFtdcRspInfoField* pRspInfo,
    int nRequestID,
    bool bIsLast) {
}

void CtpTraderSpi::OnErrRtnBatchOrderAction(CThostFtdcBatchOrderActionField* pBatchOrderAction,
                                            CThostFtdcRspInfoField* pRspInfo) {
}

void CtpTraderSpi::OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
}

/// 合约交易状态通知
void CtpTraderSpi::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField* pInstrumentStatus) {
  LOG_INF << boost::format("OnRtnInstrumentStatus: InstrumentID(%1%), ExchangeID(%2%), "
                           "InstrumentStatus(%3%)") %
             pInstrumentStatus->InstrumentID % pInstrumentStatus->ExchangeID %
             pInstrumentStatus->InstrumentStatus;
  auto instruments = ProductManager::GetInstance()->FindInstruments(
      [&](const Instrument *inst) { return inst->Product() == pInstrumentStatus->InstrumentID; });
  if (instruments.size() > 0) {
    auto req = Message::NewProto<Proto::InstrumentReq>();
    req->set_type(Proto::RequestType::Set);
    req->set_exchange(GetExchange(pInstrumentStatus->ExchangeID));
    auto status = GetInstrumentStatus(pInstrumentStatus->InstrumentStatus);
    for (auto &inst : instruments) {
      LOG_INF << boost::format("Update %1% status: %2%->%3%") % inst->Id() %
                 Proto::InstrumentStatus_Name(inst->Status()) % Proto::InstrumentStatus_Name(status);
      Instrument *instrument = const_cast<Instrument*>(inst);
      instrument->Status(status);
      auto *tmp = req->add_instruments();
      assert(tmp);
      tmp->set_id(inst->Id());
      tmp->set_type(inst->Type());
      tmp->set_status(status);
    }
    ClusterManager::GetInstance()->OnInstrumentReq(req);
  }
}

/// CTP认为投资者询价指令非法
void CtpTraderSpi::OnRspForQuoteInsert(CThostFtdcInputForQuoteField* pInputForQuote,
                                       CThostFtdcRspInfoField* pRspInfo,
                                       int nRequestID,
                                       bool bIsLast) {
}

/// 交易所认为投资者询价指令非法，若合法则无信息返回
void CtpTraderSpi::OnErrRtnForQuoteInsert(CThostFtdcInputForQuoteField* pInputForQuote,
                                          CThostFtdcRspInfoField* pRspInfo) {
}

/// 做市商接收询价
void CtpTraderSpi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField* pForQuoteRsp) {
}

std::string CtpTraderSpi::GetInstrumentId(char* id, char *exchange) {
  assert (strlen(exchange) > 2);
  std::string ret(id);
  switch (exchange[1]) {
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

Proto::Exchange CtpTraderSpi::GetExchange(const char* exchange) const {
  assert (strlen(exchange) > 2);
  switch (exchange[1]) {
    case 'C':
    case 'c':
      return Proto::Exchange::DCE;
    case 'Z':
    case 'z':
      return Proto::Exchange::CZCE;
    case 'F':
    case 'f':
      return Proto::Exchange::CFFEX;
    case 'H':
    case 'h':
      return Proto::Exchange::SHFE;
    default:
      assert(false);
  }
}

Proto::InstrumentStatus CtpTraderSpi::GetInstrumentStatus(
    TThostFtdcInstrumentStatusType status) const
{
  switch (status) {
    case THOST_FTDC_IS_BeforeTrading:
      return Proto::InstrumentStatus::PreOpen;
    case THOST_FTDC_IS_NoTrading:
      return Proto::InstrumentStatus::Halt;
    case THOST_FTDC_IS_Continous:
      return Proto::InstrumentStatus::Trading;
    case THOST_FTDC_IS_AuctionOrdering:
    case THOST_FTDC_IS_AuctionBalance:
    case THOST_FTDC_IS_AuctionMatch:
      return Proto::InstrumentStatus::OpeningAuction;
    case THOST_FTDC_IS_Closed:
      return Proto::InstrumentStatus::Closed;
    default:
      return Proto::InstrumentStatus::Unknown;
  }
}

void CtpTraderSpi::UpdateOrder(const char *exchange_id,
                               base::VolumeType volume,
                               Proto::OrderStatus status) {
  auto ord = OrderManager::GetInstance()->FindOrder(exchange_id);
  if (ord) {
    auto update = Message::NewOrder(ord);
    update->executed_volume = volume;
    update->status = status;
    api_->OnOrderResponse(update);
  } else {
    LOG_ERR << "Failed to find order by exchange id " << exchange_id;
  }
}
