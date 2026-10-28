include(FetchContent)

# TinyXML-2: scenario and catalog persistence (ev_persistence, ev_catalog).
set(tinyxml2_BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG        10.0.0
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(tinyxml2)

if(EV_BUILD_TESTS)
    # Use the shared CRT on MSVC so GoogleTest links with the project defaults.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(googletest)
endif()
