#include "ev/domain/Validation.h"

#include <algorithm>
#include <cmath>

namespace ev::domain {
namespace {

bool isSoc(double value) { return std::isfinite(value) && value >= 0.0 && value <= 100.0; }
bool isPositive(double value) { return std::isfinite(value) && value > 0.0; }
bool isNonNegative(double value) { return std::isfinite(value) && value >= 0.0; }

void validateBattery(const BatterySpec& battery, std::vector<ValidationError>& errors) {
    if (!isPositive(battery.usableCapacity.value())) {
        errors.push_back(makeError(codes::kCapacityNotPositive, {fields::kUsableCapacity}));
    }
    const bool currentOk = isSoc(battery.currentSoc.value());
    const bool targetOk = isSoc(battery.targetSoc.value());
    if (!currentOk) {
        errors.push_back(makeError(codes::kSocOutOfRange, {fields::kCurrentSoc}));
    }
    if (!targetOk) {
        errors.push_back(makeError(codes::kSocOutOfRange, {fields::kTargetSoc}));
    }
    if (currentOk && targetOk && battery.targetSoc.value() <= battery.currentSoc.value()) {
        errors.push_back(makeError(codes::kTargetNotAboveCurrent, {fields::kCurrentSoc, fields::kTargetSoc}));
    }
    const double eta = battery.efficiency;
    if (!std::isfinite(eta) || eta < 0.5 || eta > 1.0) {
        errors.push_back(makeError(codes::kEfficiencyOutOfRange, {fields::kEfficiency}));
    }
}

void validatePower(const ChargeRequest& request, std::vector<ValidationError>& errors) {
    if (!isPositive(request.vehicleMaxPower.value())) {
        errors.push_back(makeError(codes::kPowerNotPositive, {fields::kVehicleMaxPower}));
    }
    if (!isPositive(request.charger.ratedPower.value())) {
        errors.push_back(makeError(codes::kPowerNotPositive, {fields::kChargerPower}));
    }
    if (request.charger.installationLimit && !isPositive(request.charger.installationLimit->value())) {
        errors.push_back(makeError(codes::kPowerNotPositive, {fields::kInstallationLimit}));
    }
}

void validateEfficiency(const VehicleEfficiency& efficiency, std::vector<ValidationError>& errors) {
    if (!isPositive(efficiency.consumptionKwhPer100Km)) {
        errors.push_back(makeError(codes::kConsumptionNotPositive, {fields::kConsumption}));
    }
    if (efficiency.ratedRange && !isPositive(efficiency.ratedRange->value())) {
        errors.push_back(makeError(codes::kRatedRangeNotPositive, {fields::kRatedRange}));
    }
}

void validateTariff(const Tariff& tariff, std::vector<ValidationError>& errors) {
    if (!isNonNegative(tariff.energyRatePerKwh)) {
        errors.push_back(makeError(codes::kNegativeTariff, {fields::kEnergyRate}));
    }
    if (!isNonNegative(tariff.fixedFee.value())) {
        errors.push_back(makeError(codes::kNegativeTariff, {fields::kFixedFee}));
    }
    if (!isNonNegative(tariff.ratePerMinute)) {
        errors.push_back(makeError(codes::kNegativeTariff, {fields::kMinuteRate}));
    }
}

void validateCurve(const ChargeCurve& curve, std::vector<ValidationError>& errors) {
    bool bandsOk = true;
    bool powerOk = true;
    for (const auto& segment : curve.segments) {
        const double from = segment.fromSoc.value();
        const double to = segment.toSoc.value();
        if (!isSoc(from) || !isSoc(to) || from >= to) {
            bandsOk = false;
        }
        if (!isPositive(segment.maxPower.value())) {
            powerOk = false;
        }
    }
    if (bandsOk) {
        auto sorted = curve.segments;
        std::sort(sorted.begin(), sorted.end(), [](const CurveSegment& a, const CurveSegment& b) {
            return a.fromSoc.value() < b.fromSoc.value();
        });
        for (std::size_t i = 1; i < sorted.size(); ++i) {
            if (sorted[i].fromSoc.value() < sorted[i - 1].toSoc.value()) {
                bandsOk = false;
            }
        }
    }
    if (!bandsOk) {
        errors.push_back(makeError(codes::kInvalidCurve, {fields::kCurve}));
    }
    if (!powerOk) {
        errors.push_back(makeError(codes::kPowerNotPositive, {fields::kCurve}));
    }
}

}  // namespace

std::vector<ValidationError> validate(const ChargeRequest& request) {
    std::vector<ValidationError> errors;
    validateBattery(request.battery, errors);
    validatePower(request, errors);
    validateEfficiency(request.efficiency, errors);
    validateTariff(request.tariff, errors);
    validateCurve(request.curve, errors);
    return errors;
}

}  // namespace ev::domain
