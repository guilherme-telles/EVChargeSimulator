#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "TestData.h"
#include "ev/app/SimulationService.h"
#include "ev/calculator/ChargeCalculator.h"
#include "ev/persistence/XmlScenarioRepository.h"
#include "ev/persistence/XmlSupport.h"

namespace fs = std::filesystem;
using namespace ev::domain;
using ev::persistence::XmlScenarioRepository;
using ev::test::hasCode;
using ev::test::hasWarning;

namespace {

fs::path fixture(const char* name) { return fs::path(EV_FIXTURES_DIR) / name; }

std::string readAll(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

class XmlScenarioRepositoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        static std::atomic<int> counter{0};
        dir = fs::temp_directory_path() / ("ev_it_" + std::to_string(counter++) + "_" +
                                           ::testing::UnitTest::GetInstance()->current_test_info()->name());
        fs::remove_all(dir);
        fs::create_directories(dir);
    }
    void TearDown() override {
        std::error_code ec;
        fs::remove_all(dir, ec);
    }

    XmlScenarioRepository repository;
    fs::path dir;
};

}  // namespace

TEST_F(XmlScenarioRepositoryTest, Load_NormativeSpecExample_Succeeds) {
    const auto result = repository.load(fixture("reference_scenario.xml"));
    ASSERT_TRUE(result.ok()) << (result.errors.empty() ? "" : result.errors.front().detail);
    const ChargeRequest& s = *result.scenario;
    EXPECT_DOUBLE_EQ(s.battery.usableCapacity.value(), 60.0);
    EXPECT_DOUBLE_EQ(s.vehicleMaxPower.value(), 7.4);
    EXPECT_DOUBLE_EQ(s.efficiency.consumptionKwhPer100Km, 16.0);
    EXPECT_EQ(s.charger.type, ChargerType::AC);
    EXPECT_DOUBLE_EQ(s.charger.ratedPower.value(), 11.0);
    ASSERT_TRUE(s.charger.installationLimit.has_value());
    EXPECT_DOUBLE_EQ(s.charger.installationLimit->value(), 11.0);
    EXPECT_DOUBLE_EQ(s.battery.currentSoc.value(), 20.0);
    EXPECT_DOUBLE_EQ(s.battery.targetSoc.value(), 80.0);
    EXPECT_DOUBLE_EQ(s.battery.efficiency, 0.90);
    EXPECT_DOUBLE_EQ(s.tariff.energyRatePerKwh, 0.95);
    EXPECT_TRUE(result.warnings.empty());
}

// IT-001
TEST_F(XmlScenarioRepositoryTest, SaveThenLoad_RoundTrip_IsSemanticallyEquivalent) {
    auto original = ev::test::referenceRequest();
    original.vehicle = {"meu-carro", "Meu carro"};
    original.charger.type = ChargerType::DC;
    original.charger.installationLimit = Kilowatts{9.2};
    original.efficiency.ratedRange = Kilometers{410.5};
    original.tariff.fixedFee = Brl{3.5};
    original.tariff.ratePerMinute = 0.12;
    original.curve.segments = {{Percent{80.0}, Percent{100.0}, Kilowatts{3.3}}};
    const fs::path file = dir / "scenario.xml";

    ASSERT_TRUE(repository.save(original, file).empty());
    const auto loaded = repository.load(file);
    ASSERT_TRUE(loaded.ok());
    const ChargeRequest& s = *loaded.scenario;

    EXPECT_EQ(s.vehicle.id, original.vehicle.id);
    EXPECT_EQ(s.vehicle.name, original.vehicle.name);
    EXPECT_EQ(s.battery.usableCapacity, original.battery.usableCapacity);
    EXPECT_EQ(s.battery.currentSoc, original.battery.currentSoc);
    EXPECT_EQ(s.battery.targetSoc, original.battery.targetSoc);
    EXPECT_EQ(s.battery.efficiency, original.battery.efficiency);
    EXPECT_EQ(s.vehicleMaxPower, original.vehicleMaxPower);
    EXPECT_EQ(s.efficiency.consumptionKwhPer100Km, original.efficiency.consumptionKwhPer100Km);
    EXPECT_EQ(s.efficiency.ratedRange, original.efficiency.ratedRange);
    EXPECT_EQ(s.charger.type, original.charger.type);
    EXPECT_EQ(s.charger.ratedPower, original.charger.ratedPower);
    EXPECT_EQ(s.charger.installationLimit, original.charger.installationLimit);
    EXPECT_EQ(s.tariff.energyRatePerKwh, original.tariff.energyRatePerKwh);
    EXPECT_EQ(s.tariff.fixedFee, original.tariff.fixedFee);
    EXPECT_EQ(s.tariff.ratePerMinute, original.tariff.ratePerMinute);
    ASSERT_EQ(s.curve.segments.size(), 1U);
    EXPECT_EQ(s.curve.segments[0].fromSoc, original.curve.segments[0].fromSoc);
    EXPECT_EQ(s.curve.segments[0].toSoc, original.curve.segments[0].toSoc);
    EXPECT_EQ(s.curve.segments[0].maxPower, original.curve.segments[0].maxPower);
    EXPECT_FALSE(fs::exists(dir / "scenario.xml.tmp"));
}

// IT-002 / RNF-002 / UAT-07: rejected without touching the active scenario.
TEST_F(XmlScenarioRepositoryTest, Open_TruncatedXml_IsRejectedAndActiveScenarioPreserved) {
    const ev::calculator::ChargeCalculator calculator;
    ev::app::SimulationService service{calculator, repository};
    auto active = service.activeScenario();
    active.battery.currentSoc = Percent{42.0};
    service.setActiveScenario(active);

    const auto result = service.openScenario(fixture("truncated.xml"));
    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(hasCode(result.errors, codes::kInvalidFile));
    EXPECT_DOUBLE_EQ(service.activeScenario().battery.currentSoc.value(), 42.0);
}

// IT-003
TEST_F(XmlScenarioRepositoryTest, Load_MissingRequiredAttribute_ReportsFieldPath) {
    const auto result = repository.load(fixture("missing_target_soc.xml"));
    EXPECT_FALSE(result.ok());
    ASSERT_FALSE(result.errors.empty());
    EXPECT_EQ(result.errors.front().code, codes::kInvalidFile);
    EXPECT_EQ(result.errors.front().fields.front(), "scenario/battery@targetSoc");
}

// IT-004 (ADR-0005): a single decimal comma is normalized and reported.
TEST_F(XmlScenarioRepositoryTest, Load_DecimalComma_IsNormalizedWithWarning) {
    const auto result = repository.load(fixture("comma_decimal.xml"));
    ASSERT_TRUE(result.ok());
    EXPECT_DOUBLE_EQ(result.scenario->vehicleMaxPower.value(), 7.4);
    EXPECT_DOUBLE_EQ(result.scenario->battery.efficiency, 0.90);
    EXPECT_TRUE(hasWarning(result.warnings, codes::kWarnDecimalComma));
}

TEST_F(XmlScenarioRepositoryTest, Load_AmbiguousNumber_IsRejected) {
    const auto result = repository.load(fixture("ambiguous_number.xml"));
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errors.front().fields.front(), "scenario/vehicle/usableCapacity");
}

TEST(CanonicalNumber, Parse_AcceptsDotOrSingleCommaAndRejectsAmbiguity) {
    using ev::persistence::parseCanonicalNumber;
    EXPECT_DOUBLE_EQ(*parseCanonicalNumber("7.4").value, 7.4);
    EXPECT_DOUBLE_EQ(*parseCanonicalNumber(" 7,4 ").value, 7.4);
    EXPECT_TRUE(parseCanonicalNumber("7,4").normalizedComma);
    EXPECT_FALSE(parseCanonicalNumber("1,234.5").value.has_value());
    EXPECT_FALSE(parseCanonicalNumber("1,2,3").value.has_value());
    EXPECT_FALSE(parseCanonicalNumber("abc").value.has_value());
    EXPECT_FALSE(parseCanonicalNumber("7.4kW").value.has_value());
    EXPECT_FALSE(parseCanonicalNumber("").value.has_value());
}

// IT-005
TEST_F(XmlScenarioRepositoryTest, Load_FutureSchemaVersion_IsRejectedAskingForUpdate) {
    const auto result = repository.load(fixture("future_version.xml"));
    EXPECT_FALSE(result.ok());
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors.front().detail.find("atualize"), std::string::npos);
}

// IT-006
TEST_F(XmlScenarioRepositoryTest, Load_FileAbove1MB_IsRejectedBeforeParsing) {
    const fs::path file = dir / "big.xml";
    {
        std::ofstream out(file, std::ios::binary);
        out << "<scenario schemaVersion=\"1.0\">";
        out << std::string(static_cast<std::size_t>(ev::persistence::kMaxFileBytes), ' ');
        out << "</scenario>";
    }
    const auto result = repository.load(file);
    EXPECT_FALSE(result.ok());
    EXPECT_NE(result.errors.front().detail.find("1 MB"), std::string::npos);
}

// IT-007
TEST_F(XmlScenarioRepositoryTest, Load_UnknownElements_AreIgnoredWithWarning) {
    const auto result = repository.load(fixture("unknown_elements.xml"));
    ASSERT_TRUE(result.ok());
    EXPECT_TRUE(hasWarning(result.warnings, codes::kWarnUnknownElement));
    EXPECT_EQ(result.warnings.size(), 2U);
}

// IT-008
TEST_F(XmlScenarioRepositoryTest, Save_WhenWriteFails_PreservesPreviousFile) {
    const fs::path file = dir / "scenario.xml";
    ASSERT_TRUE(repository.save(ev::test::referenceRequest(), file).empty());
    const std::string before = readAll(file);
    fs::create_directories(dir / "scenario.xml.tmp");  // forces the temporary write to fail

    auto changed = ev::test::referenceRequest();
    changed.battery.targetSoc = Percent{100.0};
    const auto errors = repository.save(changed, file);
    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front().code, codes::kSaveFailed);
    EXPECT_EQ(readAll(file), before);
}

TEST_F(XmlScenarioRepositoryTest, Save_InvalidScenario_IsRejectedWithValidationErrors) {
    auto invalid = ev::test::referenceRequest();
    invalid.battery.targetSoc = Percent{10.0};
    const fs::path file = dir / "invalid.xml";
    EXPECT_TRUE(hasCode(repository.save(invalid, file), codes::kTargetNotAboveCurrent));
    EXPECT_FALSE(fs::exists(file));
}

// IT-009
TEST_F(XmlScenarioRepositoryTest, LoadAndSave_Utf8Names_ArePreserved) {
    const auto loaded = repository.load(fixture("utf8_name.xml"));
    ASSERT_TRUE(loaded.ok());
    EXPECT_EQ(loaded.scenario->vehicle.name, "Sedã Elétrico Ação São João");

    const fs::path file = dir / "utf8.xml";
    ASSERT_TRUE(repository.save(*loaded.scenario, file).empty());
    EXPECT_EQ(repository.load(file).scenario->vehicle.name, "Sedã Elétrico Ação São João");
}

TEST(XmlScenarioParse, Parse_RejectsStructuralProblems) {
    EXPECT_FALSE(XmlScenarioRepository::parse("<other schemaVersion=\"1.0\"/>").ok());
    EXPECT_FALSE(XmlScenarioRepository::parse("<scenario/>").ok());
    EXPECT_FALSE(XmlScenarioRepository::parse("<scenario schemaVersion=\"0.9\"/>").ok());
    EXPECT_FALSE(XmlScenarioRepository::parse("<scenario schemaVersion=\"1.0\"/>").ok());
    EXPECT_FALSE(XmlScenarioRepository::parse("").ok());
}

TEST(XmlScenarioParse, Parse_RejectsExcessiveDepth) {
    std::string xml = "<scenario schemaVersion=\"1.0\">";
    for (int i = 0; i < 20; ++i) {
        xml += "<a>";
    }
    for (int i = 0; i < 20; ++i) {
        xml += "</a>";
    }
    xml += "</scenario>";
    const auto result = XmlScenarioRepository::parse(xml);
    EXPECT_FALSE(result.ok());
    EXPECT_NE(result.errors.front().detail.find("profundidade"), std::string::npos);
}

TEST(XmlScenarioParse, Parse_RejectsWrongUnitChargerTypeAndCurrency) {
    auto xml = XmlScenarioRepository::serialize(ev::test::referenceRequest());
    auto replace = [](std::string text, const std::string& from, const std::string& to) {
        text.replace(text.find(from), from.size(), to);
        return text;
    };
    EXPECT_FALSE(XmlScenarioRepository::parse(replace(xml, "unit=\"kWh\"", "unit=\"Wh\"")).ok());
    EXPECT_FALSE(XmlScenarioRepository::parse(replace(xml, "type=\"AC\"", "type=\"XX\"")).ok());
    EXPECT_FALSE(XmlScenarioRepository::parse(replace(xml, "currency=\"BRL\"", "currency=\"USD\"")).ok());
}

TEST(XmlScenarioParse, Parse_SemanticallyInvalidValues_ReturnIo001PlusValidationErrors) {
    auto invalid = ev::test::referenceRequest();
    invalid.battery.targetSoc = Percent{10.0};
    const auto result = XmlScenarioRepository::parse(XmlScenarioRepository::serialize(invalid));
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errors.front().code, codes::kInvalidFile);
    EXPECT_TRUE(hasCode(result.errors, codes::kTargetNotAboveCurrent));
}

TEST(XmlScenarioParse, Load_MissingFile_ReturnsIo001) {
    const auto result = XmlScenarioRepository{}.load("does-not-exist-ev.xml");
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errors.front().code, codes::kInvalidFile);
}
