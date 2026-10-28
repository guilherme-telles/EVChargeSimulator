include(GoogleTest)

# Unit: formulas, validation, strategies, formatting (spec 8.2).
add_executable(ev_unit_tests
    unit/CalculatorTest.cpp
    unit/ValidationTest.cpp
    unit/TariffAndCurveTest.cpp
    unit/FormattingTest.cpp)
target_include_directories(ev_unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/common)
target_link_libraries(ev_unit_tests PRIVATE ev::domain ev::calculator ev::app GTest::gtest_main)
gtest_discover_tests(ev_unit_tests)

# Component: application service and DTOs, no real UI; mocks only at the persistence boundary.
add_executable(ev_component_tests
    component/SimulationServiceTest.cpp)
target_include_directories(ev_component_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/common)
target_link_libraries(ev_component_tests PRIVATE ev::app GTest::gmock_main)
gtest_discover_tests(ev_component_tests)

# Integration: XML persistence and catalog with fixture files (spec 9.1).
add_executable(ev_integration_tests
    integration/XmlScenarioRepositoryTest.cpp
    integration/VehicleCatalogTest.cpp)
target_include_directories(ev_integration_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/common)
target_link_libraries(ev_integration_tests PRIVATE ev::persistence ev::catalog ev::app GTest::gtest_main)
target_compile_definitions(ev_integration_tests PRIVATE
    EV_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/integration/fixtures"
    EV_CATALOG_FILE="${EV_CATALOG_FILE}")
gtest_discover_tests(ev_integration_tests)
