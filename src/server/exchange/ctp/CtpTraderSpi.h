#ifndef CTP_TRADER_SPI_H
#define CTP_TRADER_SPI_H

#include <unordered_map>
#include "model/Instrument.h"
#include "model/Order.h"
#include "3rd_library/ctp/include/ThostFtdcTraderApi.h"

class CtpTraderApi;
class CtpTraderSpi : public CThostFtdcTraderSpi {
  struct InstrumentConfig {
    std::string hedge_underlying;
    std::string product;
  };
  typedef std::unordered_map<std::string, InstrumentConfig> InstrumentConfigMap;

 public:
  CtpTraderSpi(CtpTraderApi* api);

  virtual void OnFrontConnected() override;
  virtual void OnFrontDisconnected(int nReason) override;
  virtual void OnHeartBeatWarning(int nTimeLapse) override;

  virtual void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin,
                              CThostFtdcRspInfoField* pRspInfo,
                              int nRequestID, bool bIsLast) override;
  virtual void OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout,
                               CThostFtdcRspInfoField* pRspInfo,
                               int nRequestID, bool bIsLast) override;
  virtual void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField* pSettlementInfo,
                                      CThostFtdcRspInfoField* pRspInfo,
                                      int nRequestID, bool bIsLast) override;
  virtual void OnRspSettlementInfoConfirm(
      CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm,
      CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
  virtual void OnRspQrySettlementInfoConfirm(
      CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm,
      CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
  virtual void OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument,
                                  CThostFtdcRspInfoField* pRspInfo,
                                  int nRequestID, bool bIsLast) override;
	virtual void OnRspQryInstrumentCommissionRate(
      CThostFtdcInstrumentCommissionRateField* pInstrumentCommissionRate,
      CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
	virtual void OnRspQryOptionInstrCommRate(CThostFtdcOptionInstrCommRateField* pOptionInstrCommRate,
                                           CThostFtdcRspInfoField* pRspInfo,
                                           int nRequestID, bool bIsLast) override;
	virtual void OnRspQryMMOptionInstrCommRate(
      CThostFtdcMMOptionInstrCommRateField* pMMOptionInstrCommRate,
      CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
  virtual void OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData,
                                       CThostFtdcRspInfoField* pRspInfo,
                                       int nRequestID, bool bIsLast) override;
  virtual void OnRspQryOrder(CThostFtdcOrderField* pOrder,
                             CThostFtdcRspInfoField* pRspInfo,
                             int nRequestID, bool bIsLast) override;
  virtual void OnRspQryTrade(CThostFtdcTradeField* pTrade,
                             CThostFtdcRspInfoField* pRspInfo,
                             int nRequestID, bool bIsLast) override;
  virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pInvestorPosition,
                                        CThostFtdcRspInfoField* pRspInfo,
                                        int nRequestID, bool bIsLast) override;
  virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField* pTradingAccount,
                                      CThostFtdcRspInfoField* pRspInfo,
                                      int nRequestID, bool bIsLast) override;
  /// CTP认为报单非法
  virtual void OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder,
                                CThostFtdcRspInfoField* pRspInfo,
                                int nRequestID, bool bIsLast) override;
  /// 交易所认为报单非法
  virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField* pInputOrder,
                                   CThostFtdcRspInfoField* pRspInfo) override;
  virtual void OnRtnOrder(CThostFtdcOrderField* pOrder) override;
  virtual void OnRtnTrade(CThostFtdcTradeField* pTrade) override;
  /// CTP认为Quote单非法
  virtual void OnRspQuoteInsert(CThostFtdcInputQuoteField* pInputQuote,
                                CThostFtdcRspInfoField* pRspInfo,
                                int nRequestID, bool bIsLast) override;
  /// 交易所认为Quote单非法
  virtual void OnErrRtnQuoteInsert(CThostFtdcInputQuoteField* pInputQuote,
                                   CThostFtdcRspInfoField* pRspInfo) override;
  virtual void OnRtnQuote(CThostFtdcQuoteField* pQuote) override;
  /// CTP认为撤单非法
  virtual void OnRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction,
                                CThostFtdcRspInfoField* pRspInfo,
                                int nRequestID, bool bIsLast) override;
  /// 交易所认为撤单非法
  virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField* pOrderAction,
                                   CThostFtdcRspInfoField *pRspInfo) override;
  /// CTP认为撤Quote单非法
  virtual void OnRspQuoteAction(CThostFtdcInputQuoteActionField* pInputQuoteAction,
                                CThostFtdcRspInfoField* pRspInfo,
                                int nRequestID, bool bIsLast) override;
  /// 交易所认为撤Quote单非法
  virtual void OnErrRtnQuoteAction(CThostFtdcQuoteActionField* pQuoteAction,
                                   CThostFtdcRspInfoField* pRspInfo) override;
  virtual void OnRspBatchOrderAction(CThostFtdcInputBatchOrderActionField* pInputBatchOrderAction,
                                     CThostFtdcRspInfoField* pRspInfo,
                                     int nRequestID, bool bIsLast) override;
  virtual void OnErrRtnBatchOrderAction(CThostFtdcBatchOrderActionField* pBatchOrderAction,
                                        CThostFtdcRspInfoField* pRspInfo) override;
  virtual void OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
  /// 合约交易状态通知
  virtual void OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField* pInstrumentStatus) override;
  /// CTP认为投资者询价指令非法
  virtual void OnRspForQuoteInsert(CThostFtdcInputForQuoteField* pInputForQuote,
                                   CThostFtdcRspInfoField* pRspInfo,
                                   int nRequestID, bool bIsLast) override;
  /// 交易所认为投资者询价指令非法，若合法则无信息返回
  virtual void OnErrRtnForQuoteInsert(CThostFtdcInputForQuoteField* pInputForQuote,
                                      CThostFtdcRspInfoField *pRspInfo) override;
  /// 做市商接收询价
  virtual void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField* pForQuoteRsp) override;
  static std::string GetInstrumentId(char* id, char* exchange);

 private:
  Proto::Exchange GetExchange(const char* exchange) const;
  // std::string GetInstrumentId(char* id, Exchanges exchange) const;
  Proto::InstrumentStatus GetInstrumentStatus(TThostFtdcInstrumentStatusType status) const;
  void UpdateOrder(const char* exchange_id, base::VolumeType volume, Proto::OrderStatus status);

  CtpTraderApi *api_ = nullptr;
  bool login = false;
  InstrumentConfigMap config_;
};

#endif // CTP_TRADER_SPI_H
