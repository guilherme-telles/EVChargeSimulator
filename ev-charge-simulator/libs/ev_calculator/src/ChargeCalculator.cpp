#include "ev/calculator/ChargeCalculator.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

#include "ev/calculator/ChargeCurveModel.h"
#include "ev/calculator/TariffStrategy.h"
#include "ev/domain/Validation.h"

namespace ev::calculator {

using namespace ev::domain;

namespace {

std::unique_ptr<IChargeCurveModel> makeCurveModel(const ChargeRequest& request) {
    const Kilowatts supply = supplyLimit(request.charger);
    if (request.curve.enabled()) {
        return std::make_unique<SegmentedCurveModel>(request.curve, request.vehicleMaxPower, supply);
    }
    return std::make_unique<ConstantPowerModel>(Kilowatts{std::min(request.vehicleMaxPower.value(), supply.value())});
}

/// t = Σ E_rede,segmento / P_segmento. With a constant model there is a single segment.
double integrateHours(const IChargeCurveModel& model, const BatterySpec& battery) {
    const double from = battery.currentSoc.value();
    const double to = battery.targetSoc.value();
    std::vector<double> points{from, to};
    for (const auto& bp : model.breakpoints()) {
        if (bp.value() > from && bp.value() < to) {
            points.push_back(bp.value());
        }
    }
    std::sort(points.begin(), points.end());

    double hours = 0.0;
    for (std::size_t i = 1; i < points.size(); ++i) {
        const double a = points[i - 1];
        const double b = points[i];
        if (b <= a) {
            continue;
        }
        const double gridEnergy = battery.usableCapacity.value() * (b - a) / 100.0 / battery.efficiency;
        const double power = model.effectivePower(Percent{(a + b) / 2.0}).value();
        hours += gridEnergy / power;
    }
    return hours;
}

bool allFinite(const ChargeResult& r) {
    const double values[] = {r.batteryEnergy.value(), r.gridEnergy.value(), r.effectivePower.value(),
                             r.duration.value(),      r.cost.total.value(), r.currentRange.value(),
                             r.addedRange.value(),    r.finalRange.value()};
    return std::all_of(std::begin(values), std::end(values), [](double v) { return std::isfinite(v); });
}

void applyRange(const ChargeRequest& request, ChargeResult& result) {
    const double kwhPerKm = request.efficiency.consumptionKwhPer100Km / 100.0;
    const double capacity = request.battery.usableCapacity.value();
    const double current = capacity * request.battery.currentSoc.value() / 100.0 / kwhPerKm;
    const double added = result.batteryEnergy.value() / kwhPerKm;
    double limit = capacity / kwhPerKm;
    if (request.efficiency.ratedRange) {
        limit = std::min(limit, request.efficiency.ratedRange->value());
    }
    double finalRange = current + added;
    if (finalRange > limit) {
        finalRange = limit;
        result.warnings.push_back({codes::kWarnRangeCapped,
                                   "Autonomia final limitada ao alcance nominal configurado (RN-08)."});
    }
    result.currentRange = Kilometers{current};
    result.addedRange = Kilometers{added};
    result.finalRange = Kilometers{finalRange};
}

}  // namespace

Kilowatts supplyLimit(const ChargerSpec& charger) {
    double limit = charger.ratedPower.value();
    if (charger.installationLimit) {
        limit = std::min(limit, charger.installationLimit->value());
    }
    return Kilowatts{limit};
}

CalculationOutcome ChargeCalculator::calculate(const ChargeRequest& request) const {
    auto errors = validate(request);
    if (!errors.empty()) {
        return CalculationOutcome{std::nullopt, std::move(errors)};
    }

    const BatterySpec& battery = request.battery;
    ChargeResult result;
    result.efficiency = battery.efficiency;
    result.usesChargeCurve = request.curve.enabled();
    result.batteryEnergy = KilowattHours{battery.usableCapacity.value() *
                                         (battery.targetSoc.value() - battery.currentSoc.value()) / 100.0};
    result.gridEnergy = KilowattHours{result.batteryEnergy.value() / battery.efficiency};

    const auto model = makeCurveModel(request);
    result.duration = Hours{integrateHours(*model, battery)};
    result.effectivePower = Kilowatts{result.gridEnergy.value() / result.duration.value()};
    result.cost = CompositeTariff{request.tariff}.calculate(result.gridEnergy, result.duration);

    if (!result.usesChargeCurve) {
        result.warnings.push_back({codes::kWarnConstantPower,
                                   "Curva de carga não informada: modelo de potência constante (RN-07)."});
    }
    applyRange(request, result);

    if (!allFinite(result)) {
        return CalculationOutcome{std::nullopt, {makeError(codes::kNonFiniteResult, {})}};
    }
    return CalculationOutcome{std::move(result), {}};
}

}  // namespace ev::calculator
