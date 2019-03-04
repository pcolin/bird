#include "Dimer.h"

Dimer::Dimer(const std::string &name, DeviceManager *dm)
    : Strategy(name, dm) {

}

void Dimer::OnStart() {
}

void Dimer::OnStop() {
}

void Dimer::OnPrice(const PricePtr &price) {
}

void Dimer::OnTheoMatrix(const TheoMatrixPtr &theo) {
}

void Dimer::OnOrder(const OrderPtr &order) {
}

void Dimer::OnTrade(const TradePtr &trade) {
}

bool Dimer::OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat) {
}

