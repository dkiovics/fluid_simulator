set(SOURCES
    imgui_impl_glfw.h
    imgui_impl_opengl3.h
    imgui.h
    imconfig.h
    imgui_impl_opengl3_loader.h
    imgui_internal.h
    imstb_rectpack.h
    imstb_textedit.h
    imstb_truetype.h
    imgui_impl_glfw.cpp
    imgui_impl_opengl3.cpp
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_widgets.cpp
    imgui_tables.cpp
)

add_library(imgui STATIC
    ${SOURCES}
)

target_link_libraries(imgui
    PUBLIC
        app_compiler_flags
        glfw
)

target_include_directories(imgui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
