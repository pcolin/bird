#ifndef TRADER_API_H
#define TRADER_API_H

class TraderApi
{
  public:
    virtual void Init() = 0;
    virtual void Login() = 0;
    virtual void Logout() = 0;

    virtual void NewOrder() = 0;
    virtual void AmendOrder() = 0;
    virtual void PullOrder() = 0;
    virtual void PullAll() = 0;

    void OnOrderResponse();
    void RejectOrder();
};

#endif
