set(SOURCES
    src/appControl.cpp
    src/prefixTest.cpp
    
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
        control_ui
        state_reconstruction
)
