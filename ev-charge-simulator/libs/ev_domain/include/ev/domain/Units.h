#pragma once

namespace ev::domain {

/// Strong wrapper around a double to avoid mixing units (e.g. kW vs kWh).
/// Values are kept at full precision; rounding happens only in presentation (RN-05).
template <typename Tag>
class Quantity {
public:
    constexpr Quantity() = default;
    constexpr explicit Quantity(double value) : value_(value) {}

    [[nodiscard]] constexpr double value() const { return value_; }

    friend constexpr bool operator==(Quantity lhs, Quantity rhs) { return lhs.value_ == rhs.value_; }
    friend constexpr bool operator!=(Quantity lhs, Quantity rhs) { return !(lhs == rhs); }

private:
    double value_ = 0.0;
};

struct KilowattHourTag {};
struct KilowattTag {};
struct PercentTag {};
struct BrlTag {};
struct KilometerTag {};
struct HourTag {};

using KilowattHours = Quantity<KilowattHourTag>;
using Kilowatts = Quantity<KilowattTag>;
using Percent = Quantity<PercentTag>;  ///< 0..100
using Brl = Quantity<BrlTag>;
using Kilometers = Quantity<KilometerTag>;
using Hours = Quantity<HourTag>;

}  // namespace ev::domain
