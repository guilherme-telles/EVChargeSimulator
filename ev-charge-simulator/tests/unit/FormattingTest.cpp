#include <gtest/gtest.h>

#include "ev/app/Formatting.h"

using namespace ev::app;

// UT-015: internal precision kept; output with 2 decimals, half away from zero.
TEST(Formatting, FormatBrl_RoundsToTwoDecimalsWithPtBrSeparators) {
    EXPECT_EQ(formatBrl(38.0), "R$ 38,00");
    EXPECT_EQ(formatBrl(12.344), "R$ 12,34");
    EXPECT_EQ(formatBrl(12.346), "R$ 12,35");
    EXPECT_EQ(formatBrl(1234567.891), "R$ 1.234.567,89");
    EXPECT_EQ(formatBrl(0.0), "R$ 0,00");
}

TEST(Formatting, RoundHalfAwayFromZero_HandlesSignAndNegativeZero) {
    EXPECT_DOUBLE_EQ(roundHalfAwayFromZero(2.5, 0), 3.0);
    EXPECT_DOUBLE_EQ(roundHalfAwayFromZero(-2.5, 0), -3.0);
    EXPECT_EQ(formatDecimal(-0.001, 2), "0,00");
    EXPECT_EQ(formatDecimal(-1234.5, 1), "-1.234,5");
}

// RN-06: "R$ 12,34" and "1,5 h"
TEST(Formatting, FormatHoursDecimal_UsesCommaAndTrimsZeros) {
    EXPECT_EQ(formatHoursDecimal(1.5), "1,5 h");
    EXPECT_EQ(formatHoursDecimal(40.0 / 7.4), "5,41 h");
    EXPECT_EQ(formatHoursDecimal(2.0), "2 h");
}

// RF-004 / UAT-08: 5,405 h -> 5 h 24 min
TEST(Formatting, FormatHoursMinutes_ReferenceExample) {
    EXPECT_EQ(formatHoursMinutes(40.0 / 7.4), "5 h 24 min");
    EXPECT_EQ(formatHoursMinutes(0.5), "30 min");
    EXPECT_EQ(formatHoursMinutes(1.9999), "2 h 0 min");
}

TEST(Formatting, FormatUnits_AppendUnitNextToValue) {
    EXPECT_EQ(formatKwh(36.0), "36,00 kWh");
    EXPECT_EQ(formatKw(7.4), "7,40 kW");
    EXPECT_EQ(formatKm(225.0), "225 km");
    EXPECT_EQ(formatKm(1234.6), "1.235 km");
    EXPECT_EQ(formatPercent(0.9), "90%");
    EXPECT_EQ(formatPercent(0.925), "92,5%");
}
