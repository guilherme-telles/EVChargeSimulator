#include "ev/calculator/TariffStrategy.h"

namespace ev::calculator {

domain::CostBreakdown CompositeTariff::calculate(domain::KilowattHours gridEnergy, domain::Hours duration) const {
    const double energy = gridEnergy.value() * tariff_.energyRatePerKwh;
    const double time = duration.value() * 60.0 * tariff_.ratePerMinute;
    const double fixed = tariff_.fixedFee.value();
    return domain::CostBreakdown{domain::Brl{energy}, domain::Brl{time}, domain::Brl{fixed},
                                 domain::Brl{energy + time + fixed}};
}

}  // namespace ev::calculator
