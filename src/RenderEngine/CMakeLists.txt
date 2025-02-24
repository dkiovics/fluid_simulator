set(ENGINE_SOURCES
    engine/callback.hpp
    engine/framebuffer.h
    engine/texture.h
    engine/uniforms.hpp
    engine/renderEngine.h
    engine/shaderProgram.h
    engine/framebuffer.cpp
    engine/renderEngine.cpp
    engine/shaderProgram.cpp
    engine/texture.cpp
)

set(ENGINE_UTILS_SOURCES
    engineUtils/object.h
    engineUtils/lights.hpp
    engineUtils/camera2D.hpp
    engineUtils/camera3D.hpp
)

set(GEOMETRIES_SOURCES
    geometries/geometry.h
    geometries/geometry.cpp
    geometries/basicGeometries.h
    geometries/basicGeometries.cpp
)

set(COMPUTE_SOURCES
    compute/computeProgram.h
    compute/computeProgram.cpp
    compute/computeTexture.h
    compute/computeTexture.cpp
    compute/storageBuffer.h
    compute/storageBuffer.cpp
)

add_subdirectory(glad)

add_library(render_engine STATIC
    ${ENGINE_SOURCES}
    ${ENGINE_UTILS_SOURCES}
    ${GEOMETRIES_SOURCES}
    ${COMPUTE_SOURCES}
)

target_include_directories(render_engine
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(render_engine
    PUBLIC 
        app_compiler_flags
        glfw
        glm::glm
    PRIVATE 
        glad
        spdlog
        stb_image
        tiny_obj_loader
)