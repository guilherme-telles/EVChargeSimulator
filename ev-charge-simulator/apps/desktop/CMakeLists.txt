set(CMAKE_AUTOMOC ON)

add_executable(ev_desktop
    main.cpp
    MainWindow.cpp
    MainWindow.h)

target_link_libraries(ev_desktop PRIVATE Qt6::Widgets ev::app ev::persistence ev::catalog)
set_target_properties(ev_desktop PROPERTIES WIN32_EXECUTABLE ON MACOSX_BUNDLE OFF)

# Ship the vehicle catalog next to the executable (data/vehicles.xml).
add_custom_command(TARGET ev_desktop POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:ev_desktop>/data
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EV_CATALOG_FILE} $<TARGET_FILE_DIR:ev_desktop>/data/vehicles.xml)
