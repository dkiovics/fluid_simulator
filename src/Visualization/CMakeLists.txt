set(SOURCES
	2D/src/grid.hpp
	2D/src/visuals2D.cpp
    3D/src/visuals3D.cpp
    3D/src/fluidRenderer.h
    3D/src/fluidSurface.h
    3D/src/fluidSurface.cpp
    3D/src/fluidParticles.h
    3D/src/fluidParticles.cpp
    3D/src/transparentBox.hpp
)

add_library(visualization STATIC
    ${SOURCES}
)

target_include_directories(visualization 
    PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(visualization 
    PUBLIC 
        app_compiler_flags
        glfw
        glm::glm
        render_engine
        glad
        simulator
        spdlog
        control_ui
)