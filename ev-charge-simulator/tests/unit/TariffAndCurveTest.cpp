#include <gtest/gtest.h>

#include "ev/calculator/ChargeCalculator.h"
#include "ev/calculator/ChargeCurveModel.h"
#include "ev/calculator/TariffStrategy.h"

using namespace ev::domain;
using namespace ev::calculator;

TEST(CompositeTariff, Calculate_EnergyOnly_IsEnergyTimesRate) {
    Tariff tariff;
    tariff.energyRatePerKwh = 0.95;
    const auto cost = CompositeTariff{tariff}.calculate(KilowattHours{40.0}, Hours{5.0});
    EXPECT_DOUBLE_EQ(cost.energy.value(), 38.0);
    EXPECT_DOUBLE_EQ(cost.time.value(), 0.0);
    EXPECT_DOUBLE_EQ(cost.fixed.value(), 0.0);
    EXPECT_DOUBLE_EQ(cost.total.value(), 38.0);
}

TEST(CompositeTariff, Calculate_AllComponents_SumsEnergyTimeAndFixed) {
    Tariff tariff;
    tariff.energyRatePerKwh = 2.0;
    tariff.fixedFee = Brl{3.0};
    tariff.ratePerMinute = 0.5;
    const auto cost = CompositeTariff{tariff}.calculate(KilowattHours{10.0}, Hours{1.5});
    EXPECT_DOUBLE_EQ(cost.energy.value(), 20.0);
    EXPECT_DOUBLE_EQ(cost.time.value(), 45.0);
    EXPECT_DOUBLE_EQ(cost.fixed.value(), 3.0);
    EXPECT_DOUBLE_EQ(cost.total.value(), 68.0);
}

TEST(ConstantPowerModel, EffectivePower_IsSameForAnySoc) {
    const ConstantPowerModel model{Kilowatts{7.4}};
    EXPECT_DOUBLE_EQ(model.effectivePower(Percent{0.0}).value(), 7.4);
    EXPECT_DOUBLE_EQ(model.effectivePower(Percent{99.0}).value(), 7.4);
    EXPECT_TRUE(model.breakpoints().empty());
}

TEST(SegmentedCurveModel, EffectivePower_UsesSegmentInsideAndVehicleOutside) {
    ChargeCurve curve;
    curve.segments = {{Percent{80.0}, Percent{100.0}, Kilowatts{3.0}}};
    const SegmentedCurveModel model{curve, Kilowatts{7.4}, Kilowatts{11.0}};
    EXPECT_DOUBLE_EQ(model.effectivePower(Percent{50.0}).value(), 7.4);
    EXPECT_DOUBLE_EQ(model.effectivePower(Percent{80.0}).value(), 3.0);  // [from, to)
    EXPECT_DOUBLE_EQ(model.effectivePower(Percent{99.9}).value(), 3.0);
    EXPECT_EQ(model.breakpoints().size(), 2U);
}

TEST(SegmentedCurveModel, EffectivePower_IsCappedBySupplyLimit) {
    ChargeCurve curve;
    curve.segments = {{Percent{0.0}, Percent{50.0}, Kilowatts{150.0}}};
    const SegmentedCurveModel model{curve, Kilowatts{100.0}, Kilowatts{50.0}};
    EXPECT_DOUBLE_EQ(model.effectivePower(Percent{10.0}).value(), 50.0);
    EXPECT_DOUBLE_EQ(model.effectivePower(Percent{60.0}).value(), 50.0);
}

TEST(SupplyLimit, UsesInstallationLimitOnlyWhenKnownAndLower) {
    ChargerSpec charger;
    charger.ratedPower = Kilowatts{11.0};
    EXPECT_DOUBLE_EQ(supplyLimit(charger).value(), 11.0);
    charger.installationLimit = Kilowatts{7.0};
    EXPECT_DOUBLE_EQ(supplyLimit(charger).value(), 7.0);
    charger.installationLimit = Kilowatts{22.0};
    EXPECT_DOUBLE_EQ(supplyLimit(charger).value(), 11.0);
}
