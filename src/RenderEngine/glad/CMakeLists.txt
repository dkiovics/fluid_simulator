add_library(glad STATIC
    glad.c
    glad.h
    khrplatform.h
)

target_link_libraries(glad
    PUBLIC 
        app_compiler_flags
)

target_include_directories(glad
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)