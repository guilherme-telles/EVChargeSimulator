#include "ev/calculator/ChargeCurveModel.h"

#include <algorithm>
#include <utility>

namespace ev::calculator {

using domain::Kilowatts;
using domain::Percent;

Kilowatts ConstantPowerModel::effectivePower(Percent /*soc*/) const { return power_; }

std::vector<Percent> ConstantPowerModel::breakpoints() const { return {}; }

SegmentedCurveModel::SegmentedCurveModel(domain::ChargeCurve curve, Kilowatts vehicleMaxPower,
                                         Kilowatts supplyLimit)
    : curve_(std::move(curve)), vehicleMaxPower_(vehicleMaxPower), supplyLimit_(supplyLimit) {}

Kilowatts SegmentedCurveModel::effectivePower(Percent soc) const {
    double accepted = vehicleMaxPower_.value();
    for (const auto& segment : curve_.segments) {
        if (soc.value() >= segment.fromSoc.value() && soc.value() < segment.toSoc.value()) {
            accepted = segment.maxPower.value();
            break;
        }
    }
    return Kilowatts{std::min(accepted, supplyLimit_.value())};
}

std::vector<Percent> SegmentedCurveModel::breakpoints() const {
    std::vector<Percent> points;
    points.reserve(curve_.segments.size() * 2);
    for (const auto& segment : curve_.segments) {
        points.push_back(segment.fromSoc);
        points.push_back(segment.toSoc);
    }
    return points;
}

}  // namespace ev::calculator
