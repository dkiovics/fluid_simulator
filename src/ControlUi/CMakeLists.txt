set(SOURCES
    controlUi.h
    
    src/controls.cpp
 )

add_library(ControlUi STATIC ${SOURCES})

target_include_directories(ControlUi 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ControlUi 
    PUBLIC
        app_compiler_flags
        glfw
        render_engine
        imgui
        spdlog
        simulator
        visualization
)
