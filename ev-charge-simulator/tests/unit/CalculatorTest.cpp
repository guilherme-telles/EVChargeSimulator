#include <gtest/gtest.h>

#include <cmath>

#include "TestData.h"
#include "ev/calculator/ChargeCalculator.h"

using namespace ev::domain;
using ev::calculator::ChargeCalculator;
using ev::test::hasCode;
using ev::test::hasWarning;
using ev::test::referenceRequest;
using ev::test::relTol;

namespace {

ChargeResult calculateOk(const ChargeRequest& request) {
    const auto outcome = ChargeCalculator{}.calculate(request);
    EXPECT_TRUE(outcome.ok());
    EXPECT_TRUE(outcome.errors.empty());
    return outcome.result.value_or(ChargeResult{});
}

}  // namespace

// UT-001 / UAT-08
TEST(ChargeCalculator, Calculate_ReferenceExample_MatchesSpecOracle) {
    const ChargeResult r = calculateOk(referenceRequest());
    EXPECT_NEAR(r.batteryEnergy.value(), 36.0, relTol(36.0));
    EXPECT_NEAR(r.gridEnergy.value(), 40.0, relTol(40.0));
    EXPECT_NEAR(r.effectivePower.value(), 7.4, relTol(7.4));
    EXPECT_NEAR(r.duration.value(), 40.0 / 7.4, relTol(40.0 / 7.4));
    EXPECT_NEAR(r.duration.value() * 60.0, 324.0, 1.0);
    EXPECT_NEAR(r.cost.total.value(), 38.0, 0.01);
    EXPECT_NEAR(r.addedRange.value(), 225.0, 1.0);
    EXPECT_DOUBLE_EQ(r.efficiency, 0.90);
    EXPECT_FALSE(r.usesChargeCurve);
}

// UT-002 / UAT-02
TEST(ChargeCalculator, Calculate_WhenVehicleLimitIsLower_UsesVehicleLimit) {
    const ChargeResult r = calculateOk(referenceRequest());
    EXPECT_NEAR(r.effectivePower.value(), 7.4, 1e-9);
}

TEST(ChargeCalculator, Calculate_WhenChargerLimitIsLower_UsesChargerLimit) {
    auto request = referenceRequest();
    request.vehicleMaxPower = Kilowatts{22.0};
    EXPECT_NEAR(calculateOk(request).effectivePower.value(), 11.0, 1e-9);
}

// UT-003
TEST(ChargeCalculator, Calculate_WhenInstallationLimitIsLowest_UsesInstallationLimit) {
    auto request = referenceRequest();
    request.charger.installationLimit = Kilowatts{3.7};
    const ChargeResult r = calculateOk(request);
    EXPECT_NEAR(r.effectivePower.value(), 3.7, 1e-9);
    EXPECT_NEAR(r.duration.value(), 40.0 / 3.7, relTol(40.0 / 3.7));
}

// UT-004
TEST(ChargeCalculator, Calculate_WhenTargetEqualsCurrent_ReturnsVal002AndNoResult) {
    auto request = referenceRequest();
    request.battery.targetSoc = request.battery.currentSoc;
    const auto outcome = ChargeCalculator{}.calculate(request);
    EXPECT_FALSE(outcome.ok());
    ASSERT_TRUE(hasCode(outcome.errors, codes::kTargetNotAboveCurrent));
    const auto& e = outcome.errors.front();
    EXPECT_EQ(e.fields, (std::vector<std::string>{fields::kCurrentSoc, fields::kTargetSoc}));
}

// UT-005
TEST(ChargeCalculator, Calculate_WhenTargetBelowCurrent_ReturnsVal002) {
    auto request = referenceRequest();
    request.battery.currentSoc = Percent{90.0};
    const auto outcome = ChargeCalculator{}.calculate(request);
    EXPECT_FALSE(outcome.ok());
    EXPECT_TRUE(hasCode(outcome.errors, codes::kTargetNotAboveCurrent));
}

// UT-007
TEST(ChargeCalculator, Calculate_WhenChargerPowerIsZero_ReturnsVal003WithoutDivision) {
    auto request = referenceRequest();
    request.charger.ratedPower = Kilowatts{0.0};
    const auto outcome = ChargeCalculator{}.calculate(request);
    EXPECT_FALSE(outcome.ok());
    EXPECT_TRUE(hasCode(outcome.errors, codes::kPowerNotPositive));
}

// UT-008
TEST(ChargeCalculator, Calculate_WhenEfficiencyIs100_GridEnergyEqualsBatteryEnergy) {
    auto request = referenceRequest();
    request.battery.efficiency = 1.0;
    const ChargeResult r = calculateOk(request);
    EXPECT_DOUBLE_EQ(r.gridEnergy.value(), r.batteryEnergy.value());
}

// UT-009
TEST(ChargeCalculator, Calculate_WhenEfficiencyIs50_GridEnergyIsDouble) {
    auto request = referenceRequest();
    request.battery.efficiency = 0.5;
    const ChargeResult r = calculateOk(request);
    EXPECT_NEAR(r.gridEnergy.value(), 2.0 * r.batteryEnergy.value(), 1e-9);
}

// UT-010
TEST(ChargeCalculator, Calculate_WhenTariffIsZero_CostIsZeroAndOtherOutputsValid) {
    auto request = referenceRequest();
    request.tariff.energyRatePerKwh = 0.0;
    const ChargeResult r = calculateOk(request);
    EXPECT_DOUBLE_EQ(r.cost.total.value(), 0.0);
    EXPECT_NEAR(r.gridEnergy.value(), 40.0, relTol(40.0));
    EXPECT_NEAR(r.addedRange.value(), 225.0, 1.0);
}

// UT-011
TEST(ChargeCalculator, Calculate_WithFixedAndPerMinuteFees_ComposesCostExactly) {
    auto request = referenceRequest();
    request.tariff.fixedFee = Brl{5.0};
    request.tariff.ratePerMinute = 0.10;
    const ChargeResult r = calculateOk(request);
    const double minutes = 40.0 / 7.4 * 60.0;
    EXPECT_NEAR(r.cost.energy.value(), 38.0, 1e-9);
    EXPECT_NEAR(r.cost.time.value(), minutes * 0.10, 1e-9);
    EXPECT_DOUBLE_EQ(r.cost.fixed.value(), 5.0);
    EXPECT_NEAR(r.cost.total.value(), r.cost.energy.value() + r.cost.time.value() + r.cost.fixed.value(), 1e-12);
}

// UT-012
TEST(ChargeCalculator, Calculate_WhenConsumptionIsZero_ReturnsVal005AndNoResult) {
    auto request = referenceRequest();
    request.efficiency.consumptionKwhPer100Km = 0.0;
    const auto outcome = ChargeCalculator{}.calculate(request);
    EXPECT_FALSE(outcome.ok());
    EXPECT_TRUE(hasCode(outcome.errors, codes::kConsumptionNotPositive));
}

// UT-013
TEST(ChargeCalculator, Calculate_WithSegmentedCurveAbove80_TimeIsSumOfSegments) {
    auto request = referenceRequest();
    request.battery.targetSoc = Percent{100.0};
    request.curve.segments = {{Percent{80.0}, Percent{100.0}, Kilowatts{3.0}}};
    const ChargeResult r = calculateOk(request);

    const double below80 = 60.0 * 0.60 / 0.90 / 7.4;  // vehicle constant power outside segments
    const double above80 = 60.0 * 0.20 / 0.90 / 3.0;
    EXPECT_NEAR(r.duration.value(), below80 + above80, 1e-9);
    EXPECT_TRUE(r.usesChargeCurve);
    EXPECT_FALSE(hasWarning(r.warnings, codes::kWarnConstantPower));
}

TEST(ChargeCalculator, Calculate_WithCurveAboveSupply_CapsSegmentPowerAtSupplyLimit) {
    auto request = referenceRequest();
    request.charger.type = ChargerType::DC;
    request.charger.ratedPower = Kilowatts{50.0};
    request.vehicleMaxPower = Kilowatts{100.0};
    request.curve.segments = {{Percent{0.0}, Percent{50.0}, Kilowatts{100.0}},
                              {Percent{50.0}, Percent{100.0}, Kilowatts{25.0}}};
    const ChargeResult r = calculateOk(request);
    const double expected = 60.0 * 0.30 / 0.90 / 50.0 + 60.0 * 0.30 / 0.90 / 25.0;
    EXPECT_NEAR(r.duration.value(), expected, 1e-9);
}

// RN-07
TEST(ChargeCalculator, Calculate_WithoutCurve_WarnsConstantPowerModel) {
    EXPECT_TRUE(hasWarning(calculateOk(referenceRequest()).warnings, codes::kWarnConstantPower));
}

// UT-014
TEST(ChargeCalculator, Calculate_WithExtremeValidValues_ProducesFiniteResult) {
    auto request = referenceRequest();
    request.battery.usableCapacity = KilowattHours{1.0e6};
    request.vehicleMaxPower = Kilowatts{1.0e-3};
    request.battery.currentSoc = Percent{0.0};
    request.battery.targetSoc = Percent{100.0};
    request.battery.efficiency = 0.5;
    const ChargeResult r = calculateOk(request);
    EXPECT_TRUE(std::isfinite(r.duration.value()));
    EXPECT_TRUE(std::isfinite(r.cost.total.value()));
    EXPECT_TRUE(std::isfinite(r.addedRange.value()));
}

TEST(ChargeCalculator, Calculate_WhenResultOverflows_ReturnsCalc001InsteadOfInfinity) {
    auto request = referenceRequest();
    request.battery.usableCapacity = KilowattHours{1.0e308};
    request.battery.currentSoc = Percent{0.0};
    request.battery.targetSoc = Percent{100.0};
    const auto outcome = ChargeCalculator{}.calculate(request);
    EXPECT_FALSE(outcome.ok());
    EXPECT_TRUE(hasCode(outcome.errors, codes::kNonFiniteResult));
}

// RF-006 / RN-08
TEST(ChargeCalculator, Calculate_ReportsCurrentAndFinalRange) {
    const ChargeResult r = calculateOk(referenceRequest());
    EXPECT_NEAR(r.currentRange.value(), 75.0, 1e-9);  // 60 × 0.2 / 0.16
    EXPECT_NEAR(r.finalRange.value(), 300.0, 1e-9);
}

TEST(ChargeCalculator, Calculate_WhenFinalRangeExceedsRatedRange_CapsAndWarns) {
    auto request = referenceRequest();
    request.efficiency.ratedRange = Kilometers{250.0};
    const ChargeResult r = calculateOk(request);
    EXPECT_DOUBLE_EQ(r.finalRange.value(), 250.0);
    EXPECT_NEAR(r.addedRange.value(), 225.0, 1e-9);
    EXPECT_TRUE(hasWarning(r.warnings, codes::kWarnRangeCapped));
}

// RNF-003: parameterized oracle table (relative error < 0.1%).
struct OracleCase {
    double capacity, current, target, charger, vehicle, efficiency, rate, consumption;
    double expectedGrid, expectedHours, expectedCost, expectedKm;
};

class ChargeCalculatorOracle : public ::testing::TestWithParam<OracleCase> {};

TEST_P(ChargeCalculatorOracle, Calculate_MatchesOracleWithinRelativeTolerance) {
    const OracleCase& c = GetParam();
    ChargeRequest request;
    request.battery = {KilowattHours{c.capacity}, Percent{c.current}, Percent{c.target}, c.efficiency};
    request.charger.ratedPower = Kilowatts{c.charger};
    request.vehicleMaxPower = Kilowatts{c.vehicle};
    request.tariff.energyRatePerKwh = c.rate;
    request.efficiency.consumptionKwhPer100Km = c.consumption;
    const ChargeResult r = calculateOk(request);
    EXPECT_NEAR(r.gridEnergy.value(), c.expectedGrid, relTol(c.expectedGrid));
    EXPECT_NEAR(r.duration.value(), c.expectedHours, relTol(c.expectedHours));
    EXPECT_NEAR(r.cost.total.value(), c.expectedCost, 0.01);
    EXPECT_NEAR(r.addedRange.value(), c.expectedKm, 1.0);
}

INSTANTIATE_TEST_SUITE_P(
    SpecAndHandComputed, ChargeCalculatorOracle,
    ::testing::Values(
        // Spec 2.3
        OracleCase{60, 20, 80, 11, 7.4, 0.90, 0.95, 16, 40.0, 40.0 / 7.4, 38.0, 225.0},
        // Home AC 7 kW, 50 kWh 10%->90%, 88% efficiency
        OracleCase{50, 10, 90, 7, 11, 0.88, 0.80, 15, 40.0 / 0.88, 40.0 / 0.88 / 7, 40.0 / 0.88 * 0.80, 40.0 / 0.15},
        // DC 50 kW limited by vehicle 46 kW, 77 kWh 5%->80%, 95% efficiency
        OracleCase{77, 5, 80, 50, 46, 0.95, 2.10, 18, 57.75 / 0.95, 57.75 / 0.95 / 46, 57.75 / 0.95 * 2.10,
                   57.75 / 0.18}));
