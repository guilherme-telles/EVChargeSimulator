#pragma once

#include <optional>
#include <string>
#include <vector>

#include "ev/domain/Units.h"

namespace ev::domain {

enum class ChargerType { AC, DC };

/// Battery state for one charging session. Efficiency is a fraction (0.5..1.0, RN-04).
struct BatterySpec {
    KilowattHours usableCapacity{0.0};
    Percent currentSoc{0.0};
    Percent targetSoc{0.0};
    double efficiency = 0.90;
};

struct ChargerSpec {
    ChargerType type = ChargerType::AC;
    Kilowatts ratedPower{0.0};
    std::optional<Kilowatts> installationLimit;  ///< Empty when unknown.
};

/// Range is estimated from average consumption only (spec 1.3).
struct VehicleEfficiency {
    double consumptionKwhPer100Km = 0.0;
    std::optional<Kilometers> ratedRange;  ///< Nominal range at 100%; caps the final range (RN-08).
};

/// Composite tariff (D-02): energy + fixed fee + per-minute rate. Zero disables a component.
struct Tariff {
    std::string currency = "BRL";
    double energyRatePerKwh = 0.0;
    Brl fixedFee{0.0};
    double ratePerMinute = 0.0;
};

/// SoC band [fromSoc, toSoc) where the vehicle accepts at most maxPower.
/// Replaces the vehicle's constant accepted power inside the band (ADR-0004).
struct CurveSegment {
    Percent fromSoc{0.0};
    Percent toSoc{0.0};
    Kilowatts maxPower{0.0};
};

struct ChargeCurve {
    std::vector<CurveSegment> segments;
    [[nodiscard]] bool enabled() const { return !segments.empty(); }
};

struct VehicleInfo {
    std::string id = "manual";
    std::string name;
};

/// Full simulation input. This is also the persisted "scenario".
struct ChargeRequest {
    VehicleInfo vehicle;
    BatterySpec battery;
    Kilowatts vehicleMaxPower{0.0};
    VehicleEfficiency efficiency;
    ChargerSpec charger;
    Tariff tariff;
    ChargeCurve curve;
};

struct CostBreakdown {
    Brl energy{0.0};
    Brl time{0.0};
    Brl fixed{0.0};
    Brl total{0.0};
};

struct Warning {
    std::string code;
    std::string message;
};

/// Output only; never trusted from imported files (spec 6.1).
struct ChargeResult {
    KilowattHours batteryEnergy{0.0};
    KilowattHours gridEnergy{0.0};
    Kilowatts effectivePower{0.0};  ///< Average power over the session (equals constant power when no curve).
    Hours duration{0.0};
    CostBreakdown cost;
    Kilometers currentRange{0.0};
    Kilometers addedRange{0.0};
    Kilometers finalRange{0.0};
    double efficiency = 0.0;
    bool usesChargeCurve = false;
    std::vector<Warning> warnings;
};

/// Vehicle catalog entry (RF-013).
struct Vehicle {
    std::string id;
    std::string name;
    KilowattHours usableCapacity{0.0};
    Kilowatts maxAcPower{0.0};
    Kilowatts maxDcPower{0.0};
    double consumptionKwhPer100Km = 0.0;
    Kilometers ratedRange{0.0};
    std::string source;
};

[[nodiscard]] Kilowatts acceptedPowerFor(const Vehicle& vehicle, ChargerType type);

}  // namespace ev::domain
