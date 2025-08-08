# Copyright (C) 2024 Voxelux
# 
# This software and its source code are proprietary and confidential.
# All rights reserved. No part of this software may be reproduced,
# distributed, or transmitted in any form or by any means without
# prior written permission from Voxelux.
# 
# Professional voxel editing application build configuration.

# Core library modules
add_subdirectory(core)

# Platform-specific helpers
add_subdirectory(platform)

# Canvas UI library
add_subdirectory(canvas_ui)

# Modern OpenGL executable with Canvas UI
if(OPENGL_FOUND AND glfw3_FOUND)
    message(STATUS "Building modern OpenGL application with Canvas UI system")
    
    add_executable(voxelux 
        voxelux_canvas.cpp
    )
    
    target_link_libraries(voxelux 
        voxelux_canvas_ui
        voxelux_core
        ${OPENGL_LIBRARIES}
        glfw
    )
    
    target_compile_features(voxelux PRIVATE cxx_std_20)
    target_include_directories(voxelux PRIVATE 
        ${OPENGL_INCLUDE_DIR} 
        ${CMAKE_SOURCE_DIR}/include
        ${GLM_INCLUDE_DIRS}
    )
    target_compile_definitions(voxelux PRIVATE VOXELUX_USE_CANVAS_UI)
    
    # Cross-platform OpenGL context hints
    if(APPLE)
        target_compile_definitions(voxelux PRIVATE VOXELUX_MACOS)
    elseif(WIN32)
        target_compile_definitions(voxelux PRIVATE VOXELUX_WINDOWS)
    else()
        target_compile_definitions(voxelux PRIVATE VOXELUX_LINUX)
    endif()
    
else()
    message(FATAL_ERROR "Canvas UI application requires OpenGL and GLFW - dependencies not found")
endif()