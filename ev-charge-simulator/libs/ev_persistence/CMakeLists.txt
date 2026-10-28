add_library(ev_persistence
    src/XmlScenarioRepository.cpp
    src/XmlSupport.cpp)
add_library(ev::persistence ALIAS ev_persistence)

target_include_directories(ev_persistence PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ev_persistence
    PUBLIC ev::domain
    PRIVATE tinyxml2)
