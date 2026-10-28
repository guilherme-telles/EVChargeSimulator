#pragma once

#include <vector>

#include "ev/domain/Model.h"

namespace ev::calculator {

/// Isolates constant power from a segmented curve (spec 5.3).
class IChargeCurveModel {
public:
    virtual ~IChargeCurveModel() = default;
    /// Power drawn from the supply at a given SoC (kW).
    [[nodiscard]] virtual domain::Kilowatts effectivePower(domain::Percent soc) const = 0;
    /// SoC points where the power may change; used to integrate time per segment.
    [[nodiscard]] virtual std::vector<domain::Percent> breakpoints() const = 0;
};

class ConstantPowerModel final : public IChargeCurveModel {
public:
    explicit ConstantPowerModel(domain::Kilowatts power) : power_(power) {}
    [[nodiscard]] domain::Kilowatts effectivePower(domain::Percent soc) const override;
    [[nodiscard]] std::vector<domain::Percent> breakpoints() const override;

private:
    domain::Kilowatts power_;
};

/// Inside a segment the vehicle accepts at most segment.maxPower; outside any segment it accepts
/// vehicleMaxPower. Either way the result is capped by supplyLimit (charger/installation).
class SegmentedCurveModel final : public IChargeCurveModel {
public:
    SegmentedCurveModel(domain::ChargeCurve curve, domain::Kilowatts vehicleMaxPower,
                        domain::Kilowatts supplyLimit);
    [[nodiscard]] domain::Kilowatts effectivePower(domain::Percent soc) const override;
    [[nodiscard]] std::vector<domain::Percent> breakpoints() const override;

private:
    domain::ChargeCurve curve_;
    domain::Kilowatts vehicleMaxPower_;
    domain::Kilowatts supplyLimit_;
};

}  // namespace ev::calculator
