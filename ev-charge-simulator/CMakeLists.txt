cmake_minimum_required(VERSION 3.25)

project(EvChargeSimulator
    VERSION 0.1.0
    DESCRIPTION "Simulador de tempo, custo e autonomia de recarga de veiculo eletrico (ERS-VE-001)"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(EV_BUILD_TESTS "Build GoogleTest suites" ON)
option(EV_BUILD_DESKTOP "Build the Qt 6 desktop application (requires Qt 6)" ON)

if(MSVC)
    # /utf-8: sources and user-facing messages are UTF-8 (pt-BR text).
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include(cmake/Dependencies.cmake)

add_subdirectory(libs/ev_domain)
add_subdirectory(libs/ev_calculator)
add_subdirectory(libs/ev_persistence)
add_subdirectory(libs/ev_catalog)
add_subdirectory(libs/ev_app)

if(EV_BUILD_DESKTOP)
    find_package(Qt6 6.2 COMPONENTS Widgets QUIET)
    if(Qt6_FOUND)
        add_subdirectory(apps/desktop)
    else()
        message(WARNING "Qt 6 not found: skipping apps/desktop. Set CMAKE_PREFIX_PATH to your Qt 6 install or use -DEV_BUILD_DESKTOP=OFF.")
    endif()
endif()

if(EV_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
