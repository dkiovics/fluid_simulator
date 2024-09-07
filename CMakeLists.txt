# Set the minimum required CMake version
cmake_minimum_required(VERSION 3.5)

# Set the project name
project(fluid_simulator)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CPP_LIB_DIR "C:/Users/dkiovics/Documents/C++_lib")

add_library(app_compiler_flags INTERFACE)
target_compile_features(app_compiler_flags INTERFACE cxx_std_20)

# Set the compiler flags for MSVC
if(msvc)
    add_compile_options(
        /W3
    )
endif()

# add CPP_LIB_DIR to the list of directories to CMAKE_PREFIX_PATH
set(CMAKE_PREFIX_PATH "${CPP_LIB_DIR}")

find_package(glm REQUIRED)
find_package(glfw REQUIRED)
find_package(arma REQUIRED)
find_package(stb_image REQUIRED)
find_package(spdlog REQUIRED)

add_subdirectory(src)