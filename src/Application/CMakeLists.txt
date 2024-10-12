set(SOURCES
    ui/simParamsAdvanced.h
    ui/simParamsAdvanced.cpp
    ui/simulationGui.h
    ui/simulationGui.cpp
    
    main.cpp
)

add_executable(Application ${SOURCES})

target_include_directories(Application 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Application 
    PUBLIC 
        app_compiler_flags
        glfw
        glm::glm
        render_engine
        glad
        simulator
        imgui
        spdlog
        visualization
)
