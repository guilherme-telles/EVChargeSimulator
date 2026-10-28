#include <gtest/gtest.h>

#include <limits>

#include "TestData.h"
#include "ev/domain/Validation.h"

using namespace ev::domain;
using ev::test::hasCode;
using ev::test::referenceRequest;

TEST(Validation, Validate_ReferenceExample_HasNoErrors) { EXPECT_TRUE(validate(referenceRequest()).empty()); }

// UT-006: SoC negative, above 100 or not finite -> VAL-001
class SocOutOfRange : public ::testing::TestWithParam<double> {};

TEST_P(SocOutOfRange, Validate_WhenCurrentSocInvalid_ReturnsVal001) {
    auto request = referenceRequest();
    request.battery.currentSoc = Percent{GetParam()};
    const auto errors = validate(request);
    ASSERT_TRUE(hasCode(errors, codes::kSocOutOfRange));
    EXPECT_EQ(errors.front().fields.front(), fields::kCurrentSoc);
}

TEST_P(SocOutOfRange, Validate_WhenTargetSocInvalid_ReturnsVal001) {
    auto request = referenceRequest();
    request.battery.targetSoc = Percent{GetParam()};
    const auto errors = validate(request);
    ASSERT_TRUE(hasCode(errors, codes::kSocOutOfRange));
    EXPECT_FALSE(hasCode(errors, codes::kTargetNotAboveCurrent));
}

INSTANTIATE_TEST_SUITE_P(Values, SocOutOfRange,
                         ::testing::Values(-0.01, -50.0, 100.01, 250.0, std::numeric_limits<double>::quiet_NaN(),
                                           std::numeric_limits<double>::infinity()));

// RN-04 / VAL-004
class EfficiencyOutOfRange : public ::testing::TestWithParam<double> {};

TEST_P(EfficiencyOutOfRange, Validate_WhenEfficiencyOutside50To100_ReturnsVal004) {
    auto request = referenceRequest();
    request.battery.efficiency = GetParam();
    EXPECT_TRUE(hasCode(validate(request), codes::kEfficiencyOutOfRange));
}

INSTANTIATE_TEST_SUITE_P(Values, EfficiencyOutOfRange,
                         ::testing::Values(0.0, 0.49, 1.01, std::numeric_limits<double>::quiet_NaN()));

TEST(Validation, Validate_EfficiencyBoundaries_AreAccepted) {
    auto request = referenceRequest();
    request.battery.efficiency = 0.5;
    EXPECT_TRUE(validate(request).empty());
    request.battery.efficiency = 1.0;
    EXPECT_TRUE(validate(request).empty());
}

// UT-007 / VAL-003
TEST(Validation, Validate_WhenAnyPowerNotPositive_ReturnsVal003ForThatField) {
    auto request = referenceRequest();
    request.vehicleMaxPower = Kilowatts{0.0};
    request.charger.installationLimit = Kilowatts{-1.0};
    const auto errors = validate(request);
    ASSERT_EQ(errors.size(), 2U);
    EXPECT_EQ(errors[0].fields.front(), fields::kVehicleMaxPower);
    EXPECT_EQ(errors[1].fields.front(), fields::kInstallationLimit);
}

TEST(Validation, Validate_WhenCapacityNotPositive_ReturnsVal006) {
    auto request = referenceRequest();
    request.battery.usableCapacity = KilowattHours{0.0};
    EXPECT_TRUE(hasCode(validate(request), codes::kCapacityNotPositive));
}

// RN-02 / VAL-007
TEST(Validation, Validate_WhenTariffValuesNegative_ReturnsVal007PerField) {
    auto request = referenceRequest();
    request.tariff.energyRatePerKwh = -0.1;
    request.tariff.fixedFee = Brl{-1.0};
    request.tariff.ratePerMinute = -0.5;
    const auto errors = validate(request);
    ASSERT_EQ(errors.size(), 3U);
    for (const auto& e : errors) {
        EXPECT_EQ(e.code, codes::kNegativeTariff);
    }
}

TEST(Validation, Validate_WhenRatedRangeNotPositive_ReturnsVal008) {
    auto request = referenceRequest();
    request.efficiency.ratedRange = Kilometers{0.0};
    EXPECT_TRUE(hasCode(validate(request), codes::kRatedRangeNotPositive));
}

// VAL-009
TEST(Validation, Validate_WhenCurveSegmentsOverlap_ReturnsVal009) {
    auto request = referenceRequest();
    request.curve.segments = {{Percent{0.0}, Percent{60.0}, Kilowatts{7.0}},
                              {Percent{50.0}, Percent{100.0}, Kilowatts{3.0}}};
    EXPECT_TRUE(hasCode(validate(request), codes::kInvalidCurve));
}

TEST(Validation, Validate_WhenCurveSegmentReversedOrOutOfRange_ReturnsVal009) {
    auto request = referenceRequest();
    request.curve.segments = {{Percent{80.0}, Percent{80.0}, Kilowatts{3.0}}};
    EXPECT_TRUE(hasCode(validate(request), codes::kInvalidCurve));
    request.curve.segments = {{Percent{80.0}, Percent{120.0}, Kilowatts{3.0}}};
    EXPECT_TRUE(hasCode(validate(request), codes::kInvalidCurve));
}

TEST(Validation, Validate_WhenCurveSegmentPowerNotPositive_ReturnsVal003OnCurve) {
    auto request = referenceRequest();
    request.curve.segments = {{Percent{80.0}, Percent{100.0}, Kilowatts{0.0}}};
    const auto errors = validate(request);
    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors[0].code, codes::kPowerNotPositive);
    EXPECT_EQ(errors[0].fields.front(), fields::kCurve);
}

TEST(Validation, Validate_AdjacentCurveSegments_AreAccepted) {
    auto request = referenceRequest();
    request.curve.segments = {{Percent{50.0}, Percent{100.0}, Kilowatts{3.0}},
                              {Percent{0.0}, Percent{50.0}, Kilowatts{7.0}}};
    EXPECT_TRUE(validate(request).empty());
}

TEST(Validation, MessageFor_KnownCodes_ReturnSpecMessages) {
    EXPECT_EQ(messageFor(codes::kSocOutOfRange), "Informe um nível de bateria entre 0% e 100%.");
    EXPECT_EQ(messageFor(codes::kTargetNotAboveCurrent), "O nível desejado deve ser maior que o nível atual.");
    EXPECT_EQ(messageFor(codes::kNonFiniteResult),
              "Não foi possível calcular com os valores informados. Revise os campos.");
    EXPECT_TRUE(messageFor("UNKNOWN").empty());
}

TEST(Model, AcceptedPowerFor_SelectsAcOrDcLimit) {
    Vehicle v;
    v.maxAcPower = Kilowatts{11.0};
    v.maxDcPower = Kilowatts{50.0};
    EXPECT_DOUBLE_EQ(acceptedPowerFor(v, ChargerType::AC).value(), 11.0);
    EXPECT_DOUBLE_EQ(acceptedPowerFor(v, ChargerType::DC).value(), 50.0);
}
