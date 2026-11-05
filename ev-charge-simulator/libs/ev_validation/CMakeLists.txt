add_library(ev_validation src/ValidationSuite.cpp)
add_library(ev::validation ALIAS ev_validation)
target_include_directories(ev_validation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ev_validation PUBLIC ev::calculator)
