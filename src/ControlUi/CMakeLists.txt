set(SOURCES
    src/controlUi.cpp
    src/controlRegistry.cpp
    src/controlRegistryPrivate.cpp
    controlUi.h
    controlRegistry.h
    src/controlRegistryPrivate.h
    src/controlElements.hpp
    src/controlCollection.cpp
 )

add_library(control_ui STATIC ${SOURCES} "src/controlCollection.cpp" "src/controlCollection.h")

target_include_directories(control_ui 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(control_ui 
    PUBLIC
        app_compiler_flags
        glfw
        render_engine
        imgui
        spdlog
        glm::glm
)
