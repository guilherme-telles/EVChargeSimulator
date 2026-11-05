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
add_subdirectory(libs/ev_validation)

if(EV_BUILD_DESKTOP)
    set(EV_QT_COMPONENTS Core Widgets Network WebEngineWidgets WebChannel)
    find_package(Qt6 6.2 QUIET COMPONENTS ${EV_QT_COMPONENTS})
    if(Qt6_FOUND)
        add_subdirectory(apps/desktop)
    elseif(NOT Qt6_DIR OR NOT EXISTS "${Qt6_DIR}/Qt6Config.cmake")
        message(WARNING "Qt 6 not found: skipping apps/desktop. Set CMAKE_PREFIX_PATH to your Qt 6 install or use -DEV_BUILD_DESKTOP=OFF.")
    else()
        set(EV_QT_MISSING "")
        foreach(_component IN LISTS EV_QT_COMPONENTS)
            if(NOT Qt6${_component}_FOUND)
                list(APPEND EV_QT_MISSING ${_component})
            endif()
        endforeach()
        list(JOIN EV_QT_MISSING ", " EV_QT_MISSING)
        message(WARNING "Qt 6 found at ${Qt6_DIR}, but required components are missing: ${EV_QT_MISSING}. Skipping apps/desktop. Install the missing Qt modules (WebEngineWidgets needs qtwebengine, qtwebchannel, qtpositioning and qtdeclarative) or use -DEV_BUILD_DESKTOP=OFF.")
    endif()
endif()

if(EV_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
