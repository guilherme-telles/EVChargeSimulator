#pragma once

#include <string>
#include <vector>

namespace ev::domain {

/// Standardized codes (spec 4.2). VAL-005..VAL-009, IO-002 and WARN-* are project extensions.
namespace codes {
inline constexpr const char* kSocOutOfRange = "VAL-001";
inline constexpr const char* kTargetNotAboveCurrent = "VAL-002";
inline constexpr const char* kPowerNotPositive = "VAL-003";
inline constexpr const char* kEfficiencyOutOfRange = "VAL-004";
inline constexpr const char* kConsumptionNotPositive = "VAL-005";
inline constexpr const char* kCapacityNotPositive = "VAL-006";
inline constexpr const char* kNegativeTariff = "VAL-007";
inline constexpr const char* kRatedRangeNotPositive = "VAL-008";
inline constexpr const char* kInvalidCurve = "VAL-009";
inline constexpr const char* kInvalidFile = "IO-001";
inline constexpr const char* kSaveFailed = "IO-002";
inline constexpr const char* kNonFiniteResult = "CALC-001";

inline constexpr const char* kWarnConstantPower = "WARN-001";
inline constexpr const char* kWarnRangeCapped = "WARN-002";
inline constexpr const char* kWarnUnknownElement = "WARN-003";
inline constexpr const char* kWarnDecimalComma = "WARN-004";
}  // namespace codes

/// User-facing message for a code (pt-BR, spec 4.2).
[[nodiscard]] std::string messageFor(const std::string& code);

/// Typed error. `fields` lists every input to highlight (VAL-002 highlights both SoC fields).
/// `detail` carries technical context, e.g. the XML path of a missing attribute (IT-003).
struct ValidationError {
    std::string code;
    std::vector<std::string> fields;
    std::string message;
    std::string detail;
};

[[nodiscard]] ValidationError makeError(const std::string& code, std::vector<std::string> fields,
                                        std::string detail = {});

/// Field identifiers shared by validation, persistence and UI.
namespace fields {
inline constexpr const char* kUsableCapacity = "battery.usableCapacity";
inline constexpr const char* kCurrentSoc = "battery.currentSoc";
inline constexpr const char* kTargetSoc = "battery.targetSoc";
inline constexpr const char* kEfficiency = "battery.efficiency";
inline constexpr const char* kVehicleMaxPower = "vehicle.maxChargePower";
inline constexpr const char* kConsumption = "vehicle.consumption";
inline constexpr const char* kRatedRange = "vehicle.ratedRange";
inline constexpr const char* kChargerPower = "charger.ratedPower";
inline constexpr const char* kInstallationLimit = "charger.installationLimit";
inline constexpr const char* kEnergyRate = "tariff.energyRate";
inline constexpr const char* kFixedFee = "tariff.fixedFee";
inline constexpr const char* kMinuteRate = "tariff.minuteRate";
inline constexpr const char* kCurve = "curve";
}  // namespace fields

}  // namespace ev::domain
