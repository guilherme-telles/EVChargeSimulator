#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "ev/domain/Model.h"
#include "ev/domain/ValidationError.h"

namespace ev::test {

/// Reference example from spec 2.3.
inline domain::ChargeRequest referenceRequest() {
    using namespace ev::domain;
    ChargeRequest r;
    r.battery = {KilowattHours{60.0}, Percent{20.0}, Percent{80.0}, 0.90};
    r.vehicleMaxPower = Kilowatts{7.4};
    r.efficiency.consumptionKwhPer100Km = 16.0;
    r.charger.type = ChargerType::AC;
    r.charger.ratedPower = Kilowatts{11.0};
    r.tariff.energyRatePerKwh = 0.95;
    return r;
}

inline bool hasCode(const std::vector<domain::ValidationError>& errors, const std::string& code) {
    return std::any_of(errors.begin(), errors.end(), [&](const auto& e) { return e.code == code; });
}

inline bool hasWarning(const std::vector<domain::Warning>& warnings, const std::string& code) {
    return std::any_of(warnings.begin(), warnings.end(), [&](const auto& w) { return w.code == code; });
}

/// Relative tolerance of 0.1% (RNF-003, spec 9.2).
inline double relTol(double expected) { return std::abs(expected) * 0.001; }

}  // namespace ev::test
