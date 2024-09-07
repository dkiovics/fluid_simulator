set(SIMULATOR_SOURCES
    simulator/macGrid/basicMacGrid.h
    simulator/macGrid/basicMacGrid.cpp
    simulator/macGrid/macGrid.h
    simulator/macGrid/macGrid.cpp
    simulator/macGrid/bridsonSolverGrid.h
    simulator/macGrid/bridsonSolverGrid.cpp
    simulator/macGrid/macGridCell.h
    simulator/macGrid/obstacles.hpp
    simulator/particles/hashedParticles.h
    simulator/particles/hashedParticles.cpp
    simulator/particles/particle.h
    simulator/util/compTimeForLoop.h
    simulator/util/glmExtraOps.h
    simulator/util/interpolation.h
    simulator/util/paralellDefine.h
    simulator/util/random.h
    simulator/simulator.h
    simulator/simulator.cpp
    manager/simulationManager.h
    manager/simulationManager.cpp
)

add_library(simulator STATIC
    ${SIMULATOR_SOURCES}
)

target_include_directories(simulator
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(simulator
    PUBLIC 
        app_compiler_flags
        glm::glm
    PRIVATE
        spdlog
)