#include "ev/domain/Model.h"

namespace ev::domain {

Kilowatts acceptedPowerFor(const Vehicle& vehicle, ChargerType type) {
    return type == ChargerType::DC ? vehicle.maxDcPower : vehicle.maxAcPower;
}

}  // namespace ev::domain
