set(SOURCES
	gradientEstimation/src/gradientEstPos.cpp
    gradientEstimation/src/gradientEstimation.cpp
    dataUtils/src/dataUtils.cpp
    adam/src/adam.cpp
    controller/stateReconstructionController.cpp
    gradientSmoothing/src/gradientSmoothing.cpp
    gradientVisualization/src/gradientVisualization.cpp
)

add_library(state_reconstruction STATIC
    ${SOURCES}
)

target_include_directories(state_reconstruction 
    PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(state_reconstruction 
    PUBLIC 
        app_compiler_flags
        glm::glm
        render_engine
        glad
        simulator
        spdlog
        control_ui
        visualization
)