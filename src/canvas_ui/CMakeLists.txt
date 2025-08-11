# Copyright (C) 2024 Voxelux
# 
# This software and its source code are proprietary and confidential.
# All rights reserved. No part of this software may be reproduced,
# distributed, or transmitted in any form or by any means without
# prior written permission from Voxelux.
# 
# Canvas UI library build configuration.

# Canvas UI library
add_library(voxelux_canvas_ui
    canvas_core.cpp
    canvas_window.cpp
    canvas_region.cpp
    canvas_renderer.cpp
    event_router.cpp
    font_system.cpp
    grid_3d_renderer.cpp
    ui_widgets.cpp
    viewport_3d_editor.cpp
    viewport_navigation_handler.cpp
    viewport_navigator.cpp
    navigation_widget.cpp
    camera_3d.cpp
    polyline_shader.cpp
    ../glad/glad.c  # GLAD OpenGL loader
)

target_compile_features(voxelux_canvas_ui PRIVATE cxx_std_20)

target_include_directories(voxelux_canvas_ui PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${OPENGL_INCLUDE_DIR}
    ${GLM_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
)

target_link_libraries(voxelux_canvas_ui PUBLIC
    ${OPENGL_LIBRARIES}
    glfw
    ${FREETYPE_LIBRARIES}
    voxelux_platform
)

target_compile_definitions(voxelux_canvas_ui PRIVATE 
    VOXELUX_CANVAS_UI_IMPLEMENTATION
)

# Cross-platform OpenGL context setup
if(APPLE)
    target_compile_definitions(voxelux_canvas_ui PRIVATE VOXELUX_MACOS)
    # On macOS, we need to request OpenGL 4.1 Core Profile explicitly
    target_compile_definitions(voxelux_canvas_ui PRIVATE GL_SILENCE_DEPRECATION)
elseif(WIN32)
    target_compile_definitions(voxelux_canvas_ui PRIVATE VOXELUX_WINDOWS)
else()
    target_compile_definitions(voxelux_canvas_ui PRIVATE VOXELUX_LINUX)
    target_link_libraries(voxelux_canvas_ui PRIVATE dl)
endif()