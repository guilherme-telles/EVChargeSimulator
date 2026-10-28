#include "ev/app/Formatting.h"

#include <charconv>
#include <cmath>
#include <iterator>

namespace ev::app {

double roundHalfAwayFromZero(double value, int decimals) {
    const double factor = std::pow(10.0, decimals);
    const double rounded = std::round(value * factor) / factor;
    return rounded == 0.0 ? 0.0 : rounded;  // avoid "-0,00"
}

std::string formatDecimal(double value, int decimals) {
    const double rounded = roundHalfAwayFromZero(value, decimals);
    char buffer[128];
    const auto [ptr, ec] =
        std::to_chars(std::begin(buffer), std::end(buffer), rounded, std::chars_format::fixed, decimals);
    if (ec != std::errc{}) {
        return "-";
    }
    std::string raw(std::begin(buffer), ptr);
    const bool negative = !raw.empty() && raw.front() == '-';
    if (negative) {
        raw.erase(0, 1);
    }
    const auto dot = raw.find('.');
    const std::string integer = raw.substr(0, dot);
    const std::string fraction = dot == std::string::npos ? std::string{} : raw.substr(dot + 1);

    std::string grouped;
    const std::size_t n = integer.size();
    for (std::size_t i = 0; i < n; ++i) {
        grouped.push_back(integer[i]);
        const std::size_t remaining = n - i - 1;
        if (remaining > 0 && remaining % 3 == 0) {
            grouped.push_back('.');
        }
    }
    std::string result = negative ? "-" + grouped : grouped;
    if (!fraction.empty()) {
        result += "," + fraction;
    }
    return result;
}

std::string formatDecimalTrimmed(double value, int maxDecimals) {
    std::string text = formatDecimal(value, maxDecimals);
    if (text.find(',') == std::string::npos) {
        return text;
    }
    while (!text.empty() && text.back() == '0') {
        text.pop_back();
    }
    if (!text.empty() && text.back() == ',') {
        text.pop_back();
    }
    return text;
}

std::string formatBrl(double value) { return "R$ " + formatDecimal(value, 2); }
std::string formatKwh(double value) { return formatDecimal(value, 2) + " kWh"; }
std::string formatKw(double value) { return formatDecimal(value, 2) + " kW"; }
std::string formatKm(double value) { return formatDecimal(value, 0) + " km"; }
std::string formatPercent(double fraction) { return formatDecimalTrimmed(fraction * 100.0, 1) + "%"; }
std::string formatHoursDecimal(double hours) { return formatDecimalTrimmed(hours, 2) + " h"; }

std::string formatHoursMinutes(double hours) {
    const long long totalMinutes = std::llround(hours * 60.0);
    const long long h = totalMinutes / 60;
    const long long m = totalMinutes % 60;
    if (h == 0) {
        return std::to_string(m) + " min";
    }
    return std::to_string(h) + " h " + std::to_string(m) + " min";
}

}  // namespace ev::app
