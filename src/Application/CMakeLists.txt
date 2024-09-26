set(SOURCES
    gfx/gfx2D/simulationGfx2D.hpp
    
    gfx/gfx3D/renderer/fluidSurfaceGfx.h
    gfx/gfx3D/renderer/fluidSurfaceGfx.cpp
    gfx/gfx3D/renderer/transparentBox.hpp
    gfx/gfx3D/renderer3DInterface.h
    gfx/gfx3D/renderer/diffRenderProxy.h
    gfx/gfx3D/renderer/diffRenderProxy.cpp
    gfx/gfx3D/renderer/simulationGfx3DRenderer.h
    gfx/gfx3D/renderer/simulationGfx3DRenderer.cpp
    gfx/gfx3D/simulationGfx3DController.h
    gfx/gfx3D/simulationGfx3DController.cpp

    gfx/gfxInterface.hpp
    
    ui/simParamsAdvanced.h
    ui/simParamsAdvanced.cpp
    ui/simulationGui.h
    ui/simulationGui.cpp
    gfx/gfx3D/renderer/paramInterface.h
    
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
        arma
)

add_custom_command(TARGET Application POST_BUILD
				   COMMAND ${CMAKE_COMMAND} -E copy_directory
				   ${CMAKE_CURRENT_SOURCE_DIR}/../shaders ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)
