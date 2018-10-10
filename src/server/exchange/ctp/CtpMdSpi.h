#ifndef CTP_MD_SPI_H
#define CTP_MD_SPI_H

#include "3rd_library/ctp/include/ThostFtdcMdApi.h"

class CtpMdApi;
class CtpMdSpi : public CThostFtdcMdSpi {
 public:
  CtpMdSpi(CtpMdApi *api);

  void OnFrontConnected() override;
  void OnHeartBeatWarning(int nTimeLapse) override;
  void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin,
                      CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
  void OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout,
                       CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
  void OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
  void OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument,
                          CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
  void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument,
                            CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
  void OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField* pSpecificInstrument,
                           CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
  void OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField* pSpecificInstrument,
                             CThostFtdcRspInfoField* pRspInfo,
                             int nRequestID, bool bIsLast) override;
  void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData) override;
  void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField* pForQuoteRsp) override;

 private:
  CtpMdApi *api_ = nullptr;
};

#endif // CTP_MD_SPI_H
