#include "Hitter.h"

Hitter::Hitter(const std::string &name, DeviceManager *dm)
    : Strategy(name, dm) {

}

void Hitter::OnStart() {
}

void Hitter::OnStop() {
}

void Hitter::OnPrice(const PricePtr &price) {
}

void Hitter::OnTheoMatrix(const TheoMatrixPtr &theo) {
}

void Hitter::OnOrder(const OrderPtr &order) {
}

void Hitter::OnTrade(const TradePtr &trade) {
}

bool Hitter::OnHeartbeat(const std::shared_ptr<Proto::Heartbeat> &heartbeat) {
}

