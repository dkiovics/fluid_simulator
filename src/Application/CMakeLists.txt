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

add_custom_command(TARGET Application POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/meshes"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../meshes" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/meshes"
)
