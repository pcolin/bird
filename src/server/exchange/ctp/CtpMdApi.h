#ifndef CTP_MD_API_H
#define CTP_MD_API_H

#include "../manager/MdApi.h"
#include "3rd_library/ctp/include/ThostFtdcMdApi.h"

class CtpMdSpi;
class CtpMdApi : public MdApi
{
  public:
    ~CtpMdApi();

    void Init();
    void Login();
    void Subscribe();

  private:
    CThostFtdcMdApi *api_ = nullptr;
    CtpMdSpi *spi_ = nullptr;
    int id_ = 0;
};

#endif
