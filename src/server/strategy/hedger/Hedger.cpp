#include "Hedger.h"

Hedger::Hedger(const std::string &name, DeviceManager *dm)
    : Strategy(name, dm) {

}

void Hedger::OnStart() {
}

void Hedger::OnStop() {
}

void Hedger::OnPrice(const PricePtr &price) {
}

void Hedger::OnTheoMatrix(const TheoMatrixPtr &theo) {
}

void Hedger::OnOrder(const OrderPtr &order) {
}

void Hedger::OnTrade(const TradePtr &trade) {
}

bool Hedger::OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat) {
}

