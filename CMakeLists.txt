cmake_minimum_required(VERSION 3.20)
project(Voxelux VERSION 0.1.0 LANGUAGES C CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(VOXELUX_BUILD_TESTS "Build tests" ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(include)

# Find packages
find_package(OpenGL REQUIRED)

# Find Qt6
find_package(Qt6 COMPONENTS Core Widgets OpenGL OpenGLWidgets REQUIRED)

# Third-party libraries
option(VOXELUX_USE_SYSTEM_LIBS "Use system libraries" ON)

# GLFW removed - using Qt for windowing

# GLM (OpenGL Mathematics)
if(VOXELUX_USE_SYSTEM_LIBS)
    find_package(glm QUIET)
    if(NOT glm_FOUND)
        message(STATUS "System GLM not found, using homebrew path")
        # Check homebrew paths
        set(GLM_INCLUDE_DIRS "/opt/homebrew/include" "/usr/local/include")
        # Check if GLM exists in homebrew
        if(EXISTS "/opt/homebrew/include/glm/glm.hpp")
            set(GLM_FOUND TRUE)
            message(STATUS "Found GLM in homebrew")
        endif()
    endif()
endif()

# GLAD removed - using Qt OpenGL for modern graphics

# ImGui removed - using Qt for modern UI

# Core library
add_subdirectory(src)

# Tests disabled - keeping build minimal
# if(VOXELUX_BUILD_TESTS)
#     enable_testing()
#     add_subdirectory(tests)
# endif()
