set(SOURCES
    "gfxInterface/gfxInterface.hpp"
	"gfx2D/simulationGfx2D.hpp"
    "gfx3D/renderer/headers/diffRenderProxy.h"
    "gfx3D/renderer3DInterface.h"
    "gfx3D/renderer/headers/simulationGfx3DRenderer.h"
    "gfx3D/renderer/headers/fluidSurfaceGfx.h"
    "gfx3D/renderer/headers/paramInterface.h"
    "gfx3D/renderer/headers/transparentBox.hpp"
    "gfx3D/renderer/diffRenderProxy.cpp"
    "gfx3D/renderer/simulationGfx3DRenderer.cpp"
    "gfx3D/renderer/fluidSurfaceGfx.cpp"
    "gfx3D/simulationGfx3DController.h"
    "gfx3D/simulationGfx3DController.cpp"
    "gfx3D/adam/adam.h"
    "gfx3D/adam/adam.cpp"
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
        imgui
        spdlog
        arma
)

add_custom_command(TARGET visualization POST_BUILD
				   COMMAND ${CMAKE_COMMAND} -E copy_directory
				   ${CMAKE_CURRENT_SOURCE_DIR}/shaders ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)
