add_library(ev_app
    src/Formatting.cpp
    src/ResultPresenter.cpp
    src/SimulationService.cpp)
add_library(ev::app ALIAS ev_app)

target_include_directories(ev_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ev_app PUBLIC ev::domain ev::calculator)
