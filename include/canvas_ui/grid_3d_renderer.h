/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Professional 3D Grid Renderer using OpenGL shaders
 */

#pragma once

#include "canvas_core.h"
#include "glad/gl.h"
#include <string>
#include <memory>

namespace voxel_canvas {

// Forward declaration
class Camera3D;

/**
 * Professional 3D Grid Renderer using OpenGL shaders
 * Modern procedural grid generation using fragment shader techniques
 */
class Grid3DRenderer {
public:
    Grid3DRenderer();
    ~Grid3DRenderer();
    
    bool initialize();
    void shutdown();
    
    // Render the 3D grid with proper perspective
    void render(const Camera3D& camera, const Rect2D& viewport, const CanvasTheme& theme);
    
    // Grid appearance settings
    void set_line_width(float width) { line_width_ = width; }
    void set_subdivision(float subdiv) { grid_subdivision_ = subdiv; }
    
private:
    bool load_shaders();
    bool create_grid_geometry();
    void cleanup_opengl_resources();
    
    // OpenGL resources
    GLuint shader_program_ = 0;
    GLuint vertex_shader_ = 0;
    GLuint fragment_shader_ = 0;
    
    GLuint vao_ = 0;  // Vertex Array Object
    GLuint vbo_ = 0;  // Vertex Buffer Object
    GLuint ebo_ = 0;  // Element Buffer Object
    
    // Shader uniforms
    GLint u_view_ = -1;
    GLint u_projection_ = -1;
    GLint u_model_ = -1;
    GLint u_camera_position_ = -1;
    GLint u_camera_distance_ = -1;
    GLint u_grid_color_ = -1;
    GLint u_grid_major_color_ = -1;
    GLint u_grid_axis_x_color_ = -1;
    GLint u_grid_axis_z_color_ = -1;
    GLint u_line_size_ = -1;
    GLint u_grid_scale_ = -1;
    GLint u_grid_subdivision_ = -1;
    
    // Grid appearance
    float line_width_ = 1.0f;  // Thin lines (1 pixel wide)
    float grid_scale_ = 100.0f;  // Size of the grid plane
    float grid_subdivision_ = 10.0f;  // Default to powers of 10
    
    bool initialized_ = false;
};

} // namespace voxel_canvas