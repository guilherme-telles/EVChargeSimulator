#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <vector>

#include "TestData.h"
#include "ev/app/ResultPresenter.h"
#include "ev/app/SimulationService.h"
#include "ev/calculator/ChargeCalculator.h"

using namespace ev::domain;
using namespace ev::app;
using ::testing::_;
using ::testing::Return;
using ::testing::Truly;

namespace {

class MockScenarioRepository : public IScenarioRepository {
public:
    MOCK_METHOD(std::vector<ValidationError>, save, (const ChargeRequest&, const std::filesystem::path&),
                (const, override));
    MOCK_METHOD(ScenarioLoadResult, load, (const std::filesystem::path&), (const, override));
};

class SimulationServiceTest : public ::testing::Test {
protected:
    ev::calculator::ChargeCalculator calculator;
    MockScenarioRepository repository;
    SimulationService service{calculator, repository};
};

}  // namespace

// UAT-08: reference example as shown to the user.
TEST_F(SimulationServiceTest, Simulate_ReferenceScenario_PresentsSpecValuesInPtBr) {
    const auto outcome = service.simulate(service.activeScenario());
    ASSERT_TRUE(outcome.ok());
    const ResultView v = presentResult(service.activeScenario(), *outcome.result);
    EXPECT_EQ(v.gridEnergy, "40,00 kWh");
    EXPECT_EQ(v.batteryEnergy, "36,00 kWh");
    EXPECT_EQ(v.effectivePower, "7,40 kW");
    EXPECT_EQ(v.duration, "5 h 24 min");
    EXPECT_EQ(v.durationDecimal, "5,41 h");
    EXPECT_EQ(v.totalCost, "R$ 38,00");
    EXPECT_EQ(v.addedRange, "225 km");
    EXPECT_EQ(v.efficiency, "90%");
    EXPECT_EQ(v.curveModel, "Potência constante");
    EXPECT_FALSE(v.assumptions.empty());
    ASSERT_EQ(v.warnings.size(), 1U);
    EXPECT_NE(v.warnings[0].find(codes::kWarnConstantPower), std::string::npos);
}

// UAT-03: inconsistent input shows errors and no result.
TEST_F(SimulationServiceTest, Simulate_InvalidInput_ReturnsErrorsAndNoResult) {
    auto request = service.activeScenario();
    request.battery.targetSoc = Percent{10.0};
    const auto outcome = service.simulate(request);
    EXPECT_FALSE(outcome.ok());
    EXPECT_TRUE(ev::test::hasCode(outcome.errors, codes::kTargetNotAboveCurrent));
}

// RNF-002 / IT-002 / UAT-07
TEST_F(SimulationServiceTest, OpenScenario_WhenRepositoryFails_KeepsActiveScenario) {
    auto active = service.activeScenario();
    active.battery.currentSoc = Percent{33.0};
    service.setActiveScenario(active);

    ScenarioLoadResult failed;
    failed.errors.push_back(makeError(codes::kInvalidFile, {"scenario"}, "XML malformado"));
    EXPECT_CALL(repository, load(_)).WillOnce(Return(failed));

    const auto result = service.openScenario("broken.xml");
    EXPECT_FALSE(result.ok());
    EXPECT_DOUBLE_EQ(service.activeScenario().battery.currentSoc.value(), 33.0);
}

TEST_F(SimulationServiceTest, OpenScenario_WhenValid_ReplacesActiveScenario) {
    ScenarioLoadResult loaded;
    loaded.scenario = ev::test::referenceRequest();
    loaded.scenario->battery.currentSoc = Percent{45.0};
    EXPECT_CALL(repository, load(_)).WillOnce(Return(loaded));

    EXPECT_TRUE(service.openScenario("ok.xml").ok());
    EXPECT_DOUBLE_EQ(service.activeScenario().battery.currentSoc.value(), 45.0);
}

TEST_F(SimulationServiceTest, SaveActiveScenario_DelegatesActiveScenarioToRepository) {
    auto active = service.activeScenario();
    active.battery.targetSoc = Percent{95.0};
    service.setActiveScenario(active);
    EXPECT_CALL(repository,
                save(Truly([](const ChargeRequest& r) { return r.battery.targetSoc.value() == 95.0; }), _))
        .WillOnce(Return(std::vector<ValidationError>{}));
    EXPECT_TRUE(service.saveActiveScenario("out.xml").empty());
}

// RF-008 / UAT-05
TEST_F(SimulationServiceTest, AddToComparison_ThreeScenarios_HighlightsCheapestAndFastest) {
    auto home = ev::test::referenceRequest();
    auto wallbox = home;
    wallbox.vehicleMaxPower = Kilowatts{11.0};
    wallbox.tariff.energyRatePerKwh = 1.10;
    auto dc = home;
    dc.charger.type = ChargerType::DC;
    dc.charger.ratedPower = Kilowatts{50.0};
    dc.vehicleMaxPower = Kilowatts{50.0};
    dc.tariff.energyRatePerKwh = 2.0;

    EXPECT_TRUE(service.addToComparison("Casa", home).ok());
    EXPECT_TRUE(service.addToComparison("Wallbox", wallbox).ok());
    EXPECT_TRUE(service.addToComparison("DC", dc).ok());
    ASSERT_EQ(service.comparison().size(), 3U);

    const auto h = highlight(service.comparison());
    EXPECT_EQ(h.lowestCost, 0U);
    EXPECT_EQ(h.fastest, 2U);
}

TEST_F(SimulationServiceTest, AddToComparison_InvalidScenario_IsNotAdded) {
    auto invalid = ev::test::referenceRequest();
    invalid.charger.ratedPower = Kilowatts{0.0};
    EXPECT_FALSE(service.addToComparison("Inválido", invalid).ok());
    EXPECT_TRUE(service.comparison().empty());
}

TEST_F(SimulationServiceTest, ClearComparison_RemovesAllEntries) {
    ASSERT_TRUE(service.addToComparison("A", ev::test::referenceRequest()).ok());
    service.clearComparison();
    EXPECT_TRUE(service.comparison().empty());
}

TEST(ComparisonHighlights, Highlight_WithFewerThanTwoEntries_IsEmpty) {
    const auto h = highlight({});
    EXPECT_FALSE(h.lowestCost.has_value());
    EXPECT_FALSE(h.fastest.has_value());
}

// RF-010 MVP: structured summary.
TEST(Summary, BuildSummary_ContainsKeyResultsAndDisclaimer) {
    const auto request = ev::test::referenceRequest();
    const auto outcome = ev::calculator::ChargeCalculator{}.calculate(request);
    ASSERT_TRUE(outcome.ok());
    const std::string text = buildSummary(request, *outcome.result);
    EXPECT_NE(text.find("5 h 24 min"), std::string::npos);
    EXPECT_NE(text.find("R$ 38,00"), std::string::npos);
    EXPECT_NE(text.find("225 km"), std::string::npos);
    EXPECT_NE(text.find(kEstimateDisclaimer), std::string::npos);
}

TEST(Presenter, PresentResult_WithCurve_DescribesSegmentedModel) {
    auto request = ev::test::referenceRequest();
    request.curve.segments = {{Percent{80.0}, Percent{100.0}, Kilowatts{3.0}}};
    const auto outcome = ev::calculator::ChargeCalculator{}.calculate(request);
    ASSERT_TRUE(outcome.ok());
    EXPECT_EQ(presentResult(request, *outcome.result).curveModel, "Curva segmentada (1 faixas)");
}

// RNF-001: p95 of a typical local simulation < 200 ms.
TEST(Performance, Simulate_TypicalScenario_P95Below200ms) {
    const ev::calculator::ChargeCalculator calculator;
    const auto request = ev::test::referenceRequest();
    std::vector<double> samplesMs;
    samplesMs.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        const auto start = std::chrono::steady_clock::now();
        const auto outcome = calculator.calculate(request);
        const auto end = std::chrono::steady_clock::now();
        ASSERT_TRUE(outcome.ok());
        samplesMs.push_back(std::chrono::duration<double, std::milli>(end - start).count());
    }
    std::sort(samplesMs.begin(), samplesMs.end());
    EXPECT_LT(samplesMs[949], 200.0);
}
