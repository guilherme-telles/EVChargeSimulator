add_library(ev_calculator
    src/ChargeCalculator.cpp
    src/ChargeCurveModel.cpp
    src/TariffStrategy.cpp)
add_library(ev::calculator ALIAS ev_calculator)

target_include_directories(ev_calculator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ev_calculator PUBLIC ev::domain)
