
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(ev_desktop
    main.cpp
    MainWindow.cpp
    MainWindow.h
    ValidationDashboard.cpp
    ValidationDashboard.h
    PremiumVisuals.cpp
    PremiumVisuals.h

    # Map module
    Map/MapPage.cpp
    Map/MapPage.h
    Map/MapBridge.cpp
    Map/MapBridge.h

    # Resources
    resources/map.qrc
    resources/icons/Icon.qrc
)

# Windows executable icon
if(WIN32)
    target_sources(ev_desktop PRIVATE
        resources/app.rc
    )
endif()

target_link_libraries(ev_desktop PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::WebEngineWidgets
    Qt6::WebChannel

    ev::app
    ev::persistence
    ev::catalog
    ev::validation
)

set_target_properties(ev_desktop PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE OFF
)

# Ship the vehicle catalog next to the executable.
add_custom_command(TARGET ev_desktop POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:ev_desktop>/data

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${EV_CATALOG_FILE}
        $<TARGET_FILE_DIR:ev_desktop>/data/vehicles.xml
)

# Copy the Qt runtime (DLLs, plugins, QtWebEngineProcess, WebEngine resources)
# next to the executable so it runs straight from the build tree.
if(WIN32 AND TARGET Qt6::windeployqt)
    add_custom_command(TARGET ev_desktop POST_BUILD
        COMMAND $<TARGET_FILE:Qt6::windeployqt>
            $<IF:$<CONFIG:Debug>,--debug,--release>
            --no-compiler-runtime
            --no-system-d3d-compiler
            --no-opengl-sw
            --exclude-plugins qtposition_nmea
            $<TARGET_FILE:ev_desktop>
        COMMENT "Deploying Qt runtime next to ev_desktop"
        VERBATIM
    )
endif()
