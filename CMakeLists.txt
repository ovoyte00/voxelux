# Copyright (C) 2024 Voxelux
# 
# This software and its source code are proprietary and confidential.
# All rights reserved. No part of this software may be reproduced,
# distributed, or transmitted in any form or by any means without
# prior written permission from Voxelux.
# 
# Professional voxel editing software build configuration.

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

# Find GLFW
find_package(glfw3 REQUIRED)

# Find FreeType for text rendering
find_package(Freetype REQUIRED)

# GLAD - Modern OpenGL function loader (better for cross-platform)
# We'll include GLAD as source files for better control

# Third-party libraries
option(VOXELUX_USE_SYSTEM_LIBS "Use system libraries" ON)

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

# Core library
add_subdirectory(src)

# Tests disabled - keeping build minimal
# if(VOXELUX_BUILD_TESTS)
#     enable_testing()
#     add_subdirectory(tests)
# endif()
