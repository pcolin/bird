#include "SimulationTraderApi.h"
#include "model/Middleware.h"
#include "model/Future.h"
#include "model/Option.h"
#include "model/ProductManager.h"
#include "strategy/ClusterManager.h"
#include "Position.pb.h"

#include <future>

void SimulationTraderApi::Init()
{
  auto req = Message::NewProto<Proto::InstrumentReq>();
  req->set_type(Proto::RequestType::Get);
  req->set_exchange(Proto::Exchange::DCE);
  req->set_user(EnvConfig::GetInstance()->GetString(EnvVar::EXCHANGE));
  auto r = std::dynamic_pointer_cast<Proto::InstrumentRep>(Middleware::GetInstance()->Request(req));
  if (unlikely(!r))
  {
    LOG_INF << "Failed to sync instruments";
  }
  std::unordered_map<std::string, std::tuple<std::string, std::string>> underlyings;
  std::unordered_map<std::string, Instrument*> insts;
  for (auto &inst : r->instruments())
  {
    Instrument *instrument = nullptr;
    if (inst.type() == Proto::InstrumentType::Option)
    {
      Option *o = new Option();
      o->CallPut(inst.call_put());
      o->ExerciseType(inst.exercise());
      o->SettlementType(inst.settlement());
      o->Strike(inst.strike());
      instrument = o;
    }
    else if (inst.type() == Proto::InstrumentType::Future)
    {
      Future *f = new Future();
      f->Underlying(f);
      instrument = f;
    }
    LOG_DBG << "Begin to deal " << inst.ShortDebugString();
    instrument->Id(inst.id());
    instrument->Symbol(inst.symbol());
    instrument->Exchange(inst.exchange());
    instrument->Currency(inst.currency());
    instrument->Tick(inst.tick());
    instrument->Multiplier(inst.multiplier());
    instrument->Maturity(boost::gregorian::from_undelimited_string(inst.maturity()));
    underlyings[inst.id()] = std::make_tuple(inst.underlying(), inst.hedge_underlying());

    insts.emplace(inst.id(), instrument);
  }

  for (auto &it : insts)
  {
    auto &tpl = underlyings[it.first];
    auto &underlying = std::get<0>(tpl);
    auto &hedge_underlying = std::get<1>(tpl);
    it.second->Underlying(insts[underlying]);
    it.second->HedgeUnderlying(insts[hedge_underlying]);

    ProductManager::GetInstance()->Add(it.second);
    LOG_INF << "Add instrument " << it.second->Id();

    if (it.second->Type() == Proto::InstrumentType::Future &&
        it.second->Underlying() == it.second->HedgeUnderlying())
    {
      ClusterManager::GetInstance()->AddDevice(it.second);
    }
  }

  td_ = std::make_unique<std::thread>(std::bind(&SimulationTraderApi::MatchingProcess, this));
}

void SimulationTraderApi::NewOrder(const OrderPtr &order)
{

}

void SimulationTraderApi::NewQuote(const OrderPtr &bid, const OrderPtr &ask)
{

}

void SimulationTraderApi::AmendOrder(const OrderPtr &order)
{

}

void SimulationTraderApi::AmendQuote(const OrderPtr &bid, const OrderPtr &ask)
{

}

void SimulationTraderApi::PullOrder(const OrderPtr &order)
{

}

void SimulationTraderApi::PullQuote(const OrderPtr &bid, const OrderPtr &ask)
{

}

void SimulationTraderApi::QueryCash()
{

}

void SimulationTraderApi::MatchingProcess()
{
  std::this_thread::sleep_for(std::chrono::seconds(5));
  LOG_INF << "Start matching process....";

  int i = 0;
  auto insts = ProductManager::GetInstance()->FindInstruments([](const Instrument*){ return true; });
  for (auto &inst : insts)
  {
    if (++i % 2 == 0) continue;

    auto p = Message::NewProto<Proto::Position>();
    p->set_instrument(inst->Id());

    if (i % 3)
    {
      p->set_total_long(10);
      p->set_total_short(20);
    }
    else
    {
      p->set_total_long(20);
      p->set_total_short(10);
    }
    p->set_liquid_long(5);
    p->set_yesterday_long(3);
    p->set_liquid_short(5);
    p->set_yesterday_short(3);

    ClusterManager::GetInstance()->FindDevice(inst->HedgeUnderlying())->Publish(p);
  }

  i = 0;
  while (true)
  {
    auto cash = std::make_shared<Proto::Cash>();
    cash->set_currency(Proto::CNY);
    cash->set_account("99665550");
    cash->set_total(8888888.88 + 3333333.33);
    cash->set_available(8888888.88);
    cash->set_margin(2222222.22);
    static const double limit = EnvConfig::GetInstance()->GetDouble(EnvVar::OPT_CASH_LIMIT);
    cash->set_is_enough(cash->available() >= limit);
    ClusterManager::GetInstance()->OnCash(cash);

    if (i % 12 == 0)
    {
      auto status = i % 9 ?  Proto::InstrumentStatus::Trading : Proto::InstrumentStatus::Halt;
      auto req = Message::NewProto<Proto::InstrumentReq>();
      req->set_type(Proto::RequestType::Set);
      for (auto &inst : insts)
      {
        Instrument *instrument = const_cast<Instrument*>(inst);
        instrument->Status(status);
        req->add_instruments()->set_id(inst->Id());
        req->add_instruments()->set_status(status);
      }
      ClusterManager::GetInstance()->OnInstrumentReq(req);
    }

    ++i;
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }
}
