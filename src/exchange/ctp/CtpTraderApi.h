#ifndef CTP_TRADER_API_H
#define CTP_TRADER_API_H

#include <mutex>
#include <condition_variable>

#include "../manager/TraderApi.h"
#include "3rd_library/ctp/include/ThostFtdcTraderApi.h"

class CtpTraderSpi;
class CtpTraderApi : public TraderApi
{
  public:
    ~CtpTraderApi();

    virtual void Init();
    virtual void Login();
    virtual void Logout();

    virtual void NewOrder();
    virtual void AmendOrder();
    virtual void PullOrder();
    virtual void PullAll();

    void QueryInstruments();
    void QueryOrders();
    void QueryTrades();
    void QueryPositions();
    void QueryCash();
    void NotifyInstrumentReady();

  private:
    CThostFtdcTraderApi *api_ = nullptr;
    CtpTraderSpi *spi_ = nullptr;
    int id_ = 0;

    std::mutex inst_mtx_;
    std::condition_variable inst_cv_;
    bool inst_ready_ = false;
};

#endif
