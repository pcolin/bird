#include "SimulationTraderApi.h"
#include "model/Middleware.h"
#include "model/Future.h"
#include "model/Option.h"
#include "model/ProductManager.h"
#include "strategy/ClusterManager.h"

void SimulationTraderApi::Init()
{
  auto req = Message::NewProto<Proto::InstrumentReq>();
  req->set_type(Proto::RequestType::Get);
  req->set_exchange(Proto::Exchange::DE);
  req->set_user(EnvConfig::GetInstance()->GetString(EnvVar::EXCHANGE));
  auto r = std::dynamic_pointer_cast<Proto::InstrumentRep>(Middleware::GetInstance()->Request(req));
  if (r)
  {
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

    for (auto &it: insts)
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
  }
  else
  {
    LOG_INF << "Failed to sync instruments";
  }
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