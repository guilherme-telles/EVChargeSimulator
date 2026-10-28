add_library(ev_catalog src/XmlVehicleCatalog.cpp)
add_library(ev::catalog ALIAS ev_catalog)

target_include_directories(ev_catalog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ev_catalog
    PUBLIC ev::domain
    PRIVATE ev::persistence tinyxml2)

set(EV_CATALOG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/data/vehicles.xml CACHE INTERNAL "Bundled vehicle catalog")
