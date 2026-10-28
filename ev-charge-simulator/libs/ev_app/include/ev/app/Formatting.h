#pragma once

#include <string>

namespace ev::app {

/// Rounding policy (RN-05, ADR-0006): full precision internally, round half away from zero only here.
[[nodiscard]] double roundHalfAwayFromZero(double value, int decimals);

/// pt-BR number: '.' thousands separator, ',' decimal separator (RN-06). Example: 1234.5 -> "1.234,50".
[[nodiscard]] std::string formatDecimal(double value, int decimals);
/// Same as formatDecimal but drops trailing zeros: 1.50 -> "1,5", 2.00 -> "2".
[[nodiscard]] std::string formatDecimalTrimmed(double value, int maxDecimals);

[[nodiscard]] std::string formatBrl(double value);            ///< "R$ 12,34"
[[nodiscard]] std::string formatKwh(double value);            ///< "36,00 kWh"
[[nodiscard]] std::string formatKw(double value);             ///< "7,40 kW"
[[nodiscard]] std::string formatKm(double value);             ///< "225 km"
[[nodiscard]] std::string formatPercent(double fraction);     ///< 0.9 -> "90%"
[[nodiscard]] std::string formatHoursDecimal(double hours);   ///< "5,41 h", "1,5 h"
[[nodiscard]] std::string formatHoursMinutes(double hours);   ///< "5 h 24 min"

}  // namespace ev::app
