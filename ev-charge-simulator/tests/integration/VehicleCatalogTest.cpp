#include <gtest/gtest.h>

#include "ev/catalog/XmlVehicleCatalog.h"

using namespace ev::domain;
using namespace ev::catalog;

TEST(VehicleCatalog, LoadBundledCatalog_ReturnsValidatedVehicles) {
    const auto result = loadCatalog(EV_CATALOG_FILE);
    ASSERT_TRUE(result.ok()) << (result.errors.empty() ? "" : result.errors.front().detail);
    const auto vehicles = result.catalog->list();
    ASSERT_FALSE(vehicles.empty());
    for (const auto& v : vehicles) {
        EXPECT_FALSE(v.id.empty());
        EXPECT_FALSE(v.name.empty());
        EXPECT_GT(v.usableCapacity.value(), 0.0);
        EXPECT_GT(v.maxAcPower.value(), 0.0);
        EXPECT_GT(v.maxDcPower.value(), 0.0);
        EXPECT_GT(v.consumptionKwhPer100Km, 0.0);
        EXPECT_GT(v.ratedRange.value(), 0.0);
    }
}

TEST(VehicleCatalog, Find_ReturnsVehicleByIdOrEmpty) {
    const auto result = parseCatalog(R"(<vehicleCatalog schemaVersion="1.0">
        <vehicle id="a" name="A" usableCapacityKWh="50" maxAcKw="7" maxDcKw="50" consumptionKWh100Km="15" ratedRangeKm="330"/>
    </vehicleCatalog>)");
    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.catalog->find("a").has_value());
    EXPECT_DOUBLE_EQ(result.catalog->find("a")->usableCapacity.value(), 50.0);
    EXPECT_FALSE(result.catalog->find("missing").has_value());
}

TEST(VehicleCatalog, Parse_DuplicateIds_AreRejected) {
    const auto result = parseCatalog(R"(<vehicleCatalog schemaVersion="1.0">
        <vehicle id="a" name="A" usableCapacityKWh="50" maxAcKw="7" maxDcKw="50" consumptionKWh100Km="15" ratedRangeKm="330"/>
        <vehicle id="a" name="B" usableCapacityKWh="60" maxAcKw="7" maxDcKw="50" consumptionKWh100Km="15" ratedRangeKm="330"/>
    </vehicleCatalog>)");
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errors.front().fields.front(), "vehicleCatalog/vehicle[1]@id");
}

TEST(VehicleCatalog, Parse_NonPositiveOrMissingNumbers_AreRejected) {
    EXPECT_FALSE(parseCatalog(R"(<vehicleCatalog schemaVersion="1.0">
        <vehicle id="a" name="A" usableCapacityKWh="0" maxAcKw="7" maxDcKw="50" consumptionKWh100Km="15" ratedRangeKm="330"/>
    </vehicleCatalog>)").ok());
    EXPECT_FALSE(parseCatalog(R"(<vehicleCatalog schemaVersion="1.0">
        <vehicle id="a" name="A" maxAcKw="7" maxDcKw="50" consumptionKWh100Km="15" ratedRangeKm="330"/>
    </vehicleCatalog>)").ok());
}

TEST(VehicleCatalog, Parse_StructuralProblems_AreRejected) {
    EXPECT_FALSE(parseCatalog("<vehicles/>").ok());
    EXPECT_FALSE(parseCatalog("<vehicleCatalog/>").ok());
    EXPECT_FALSE(parseCatalog("<vehicleCatalog schemaVersion=\"1.0\"><vehicle").ok());
    EXPECT_FALSE(loadCatalog("does-not-exist-catalog.xml").ok());
}
