set(SOURCES
    "gfxInterface/gfxInterface.hpp"
	"gfx2D/simulationGfx2D.hpp"
    "gfx3D/diffRenderer/diffRenderProxy.h"
    "gfx3D/renderer3DInterface.h"
    "gfx3D/renderer/headers/simulationGfx3DRenderer.h"
    "gfx3D/renderer/headers/fluidSurfaceGfx.h"
    "gfx3D/renderer/headers/paramInterface.h"
    "gfx3D/renderer/headers/transparentBox.hpp"
    "gfx3D/diffRenderer/diffRenderProxy.cpp"
    "gfx3D/renderer/simulationGfx3DRenderer.cpp"
    "gfx3D/renderer/fluidSurfaceGfx.cpp"
    "gfx3D/simulationGfx3DController.h"
    "gfx3D/simulationGfx3DController.cpp"
    "gfx3D/optimizer/adam.h"
    "gfx3D/optimizer/adam.cpp"
    "gfx3D/optimizer/densityControl.h"
    "gfx3D/optimizer/densityControl.cpp"
    "gfx3D/diffRenderer/gradientCalculatorInterface.h" 
    "gfx3D/diffRenderer/gradientCalculatorPos.h" 
    "gfx3D/diffRenderer/gradientCalculatorPos.cpp" 
    "gfx3D/diffRenderer/gradientCalculatorSpeed.h" 
    "gfx3D/diffRenderer/gradientCalculatorSpeed.cpp"
    "gfx3D/gradientSmoothing/gradientSmoothing.h" 
    "gfx3D/gradientSmoothing/gradientSmoothing.cpp"
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
)

add_custom_command(TARGET visualization POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders"
)