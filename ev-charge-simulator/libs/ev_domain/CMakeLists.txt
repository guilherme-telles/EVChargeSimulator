add_library(ev_domain
    src/Model.cpp
    src/Validation.cpp
    src/ValidationError.cpp)
add_library(ev::domain ALIAS ev_domain)

target_include_directories(ev_domain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(ev_domain PUBLIC cxx_std_17)
