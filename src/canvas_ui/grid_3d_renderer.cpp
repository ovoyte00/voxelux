/*
 * Copyright (C) 2024 Voxelux
 * 
 * Professional 3D Grid Renderer implementation
 */

#include "canvas_ui/grid_3d_renderer.h"
#include "canvas_ui/camera_3d.h"
#include "glad/gl.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

namespace voxel_canvas {

// Grid3DRenderer implementation

Grid3DRenderer::Grid3DRenderer() {
}

Grid3DRenderer::~Grid3DRenderer() {
    shutdown();
}

bool Grid3DRenderer::initialize() {
    if (initialized_) {
        return true;
    }
    
    
    if (!load_shaders()) {
        std::cerr << "ERROR: Failed to load grid shaders" << std::endl;
        return false;
    }
    
    if (!create_grid_geometry()) {
        std::cerr << "ERROR: Failed to create grid geometry" << std::endl;
        return false;
    }
    
    initialized_ = true;
    return true;
}

void Grid3DRenderer::shutdown() {
    if (!initialized_) {
        return;
    }
    
    cleanup_opengl_resources();
    initialized_ = false;
}

bool Grid3DRenderer::load_shaders() {
    
    // Load vertex shader - try multiple possible paths
    std::vector<std::string> vertex_paths = {
        "../../shaders/grid/grid.vert",     // From build/bin/
        "../shaders/grid/grid.vert",        // From build/
        "shaders/grid/grid.vert",           // From project root
        "./shaders/grid/grid.vert"          // Current directory
    };
    
    std::ifstream vertex_file;
    std::string used_path;
    
    for (const auto& path : vertex_paths) {
        vertex_file.open(path);
        if (vertex_file.is_open()) {
            used_path = path;
            break;
        }
    }
    
    if (!vertex_file.is_open()) {
        std::cerr << "ERROR: Failed to open vertex shader file at any of these paths:" << std::endl;
        for (const auto& path : vertex_paths) {
            std::cerr << "  - " << path << std::endl;
        }
        return false;
    }
    
    std::stringstream vertex_stream;
    vertex_stream << vertex_file.rdbuf();
    std::string vertex_code = vertex_stream.str();
    vertex_file.close();
    
    // Load fragment shader - try multiple possible paths
    std::vector<std::string> fragment_paths = {
        "../../shaders/grid/grid.frag",     // From build/bin/
        "../shaders/grid/grid.frag",        // From build/
        "shaders/grid/grid.frag",           // From project root
        "./shaders/grid/grid.frag"          // Current directory
    };
    
    std::ifstream fragment_file;
    
    for (const auto& path : fragment_paths) {
        fragment_file.open(path);
        if (fragment_file.is_open()) {
            break;
        }
    }
    
    if (!fragment_file.is_open()) {
        std::cerr << "ERROR: Failed to open fragment shader file at any of these paths:" << std::endl;
        for (const auto& path : fragment_paths) {
            std::cerr << "  - " << path << std::endl;
        }
        return false;
    }
    
    std::stringstream fragment_stream;
    fragment_stream << fragment_file.rdbuf();
    std::string fragment_code = fragment_stream.str();
    fragment_file.close();
    
    // Compile vertex shader
    vertex_shader_ = glCreateShader(GL_VERTEX_SHADER);
    const char* vertex_src = vertex_code.c_str();
    glShaderSource(vertex_shader_, 1, &vertex_src, nullptr);
    glCompileShader(vertex_shader_);
    
    GLint success;
    GLchar info_log[512];
    glGetShaderiv(vertex_shader_, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex_shader_, 512, nullptr, info_log);
        std::cerr << "Vertex shader compilation failed: " << info_log << std::endl;
        return false;
    }
    
    // Compile fragment shader
    fragment_shader_ = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragment_src = fragment_code.c_str();
    glShaderSource(fragment_shader_, 1, &fragment_src, nullptr);
    glCompileShader(fragment_shader_);
    
    glGetShaderiv(fragment_shader_, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment_shader_, 512, nullptr, info_log);
        std::cerr << "Fragment shader compilation failed: " << info_log << std::endl;
        return false;
    }
    
    // Create shader program
    shader_program_ = glCreateProgram();
    glAttachShader(shader_program_, vertex_shader_);
    glAttachShader(shader_program_, fragment_shader_);
    glLinkProgram(shader_program_);
    
    glGetProgramiv(shader_program_, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program_, 512, nullptr, info_log);
        std::cerr << "Shader program linking failed: " << info_log << std::endl;
        return false;
    }
    
    // Get uniform locations
    u_view_ = glGetUniformLocation(shader_program_, "view");
    u_projection_ = glGetUniformLocation(shader_program_, "projection");
    u_model_ = glGetUniformLocation(shader_program_, "model");
    u_camera_position_ = glGetUniformLocation(shader_program_, "camera_position");
    u_camera_distance_ = glGetUniformLocation(shader_program_, "camera_distance");
    u_grid_color_ = glGetUniformLocation(shader_program_, "grid_color");
    u_grid_major_color_ = glGetUniformLocation(shader_program_, "grid_major_color");
    u_grid_axis_x_color_ = glGetUniformLocation(shader_program_, "grid_axis_x_color");
    u_grid_axis_z_color_ = glGetUniformLocation(shader_program_, "grid_axis_z_color");
    u_line_size_ = glGetUniformLocation(shader_program_, "line_size");
    u_grid_scale_ = glGetUniformLocation(shader_program_, "grid_scale");
    u_grid_subdivision_ = glGetUniformLocation(shader_program_, "grid_subdivision");
    
    return true;
}

bool Grid3DRenderer::create_grid_geometry() {
    // Create a large quad for the grid plane (like Blender's approach)
    float size = grid_scale_;
    float vertices[] = {
        // Position (x, y, z), TexCoord (u, v)
        -size, 0.0f, -size,  0.0f, 0.0f,
         size, 0.0f, -size,  1.0f, 0.0f,
         size, 0.0f,  size,  1.0f, 1.0f,
        -size, 0.0f,  size,  0.0f, 1.0f
    };
    
    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };
    
    // Generate buffers
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    
    // Bind VAO
    glBindVertexArray(vao_);
    
    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // Set vertex attributes
    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinates
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    return true;
}

void Grid3DRenderer::render(const Camera3D& camera, const Rect2D& viewport, const CanvasTheme& theme) {
    if (!initialized_) {
        return;
    }
    
    
    // Set OpenGL viewport to match the rendering region
    glViewport(viewport.x, viewport.y, viewport.width, viewport.height);
    
    // Disable depth test for now to simplify debugging
    glDisable(GL_DEPTH_TEST);
    
    // Enable blending for grid transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Use grid shader
    glUseProgram(shader_program_);
    
    // Dynamic grid scaling based on camera distance - make it much larger
    float dynamic_grid_size = std::max(10000.0f, camera.get_distance() * 100.0f);
    
    // Set camera aspect ratio
    const_cast<Camera3D&>(camera).set_aspect_ratio(viewport.width / viewport.height);
    
    // Get matrices from professional Camera3D system
    Matrix4x4 view_matrix = camera.get_view_matrix();
    Matrix4x4 projection_matrix = camera.get_projection_matrix();
    
    
    // Create model matrix for grid scaling
    Matrix4x4 model_matrix = Matrix4x4::scale({dynamic_grid_size/grid_scale_, 1.0f, dynamic_grid_size/grid_scale_});
    
    // OpenGL expects column-major matrices, but ours are row-major, so transpose
    glUniformMatrix4fv(u_view_, 1, GL_TRUE, view_matrix.data());
    glUniformMatrix4fv(u_projection_, 1, GL_TRUE, projection_matrix.data());
    glUniformMatrix4fv(u_model_, 1, GL_TRUE, model_matrix.data());
    
    // Set camera uniforms
    if (u_camera_position_ != -1) {
        Vector3D cam_pos = camera.get_position();
        glUniform3f(u_camera_position_, cam_pos.x, cam_pos.y, cam_pos.z);
    }
    if (u_camera_distance_ != -1) {
        glUniform1f(u_camera_distance_, camera.get_distance());
    }
    
    // Set grid appearance uniforms using theme colors
    if (u_grid_color_ != -1) {
        glUniform4f(u_grid_color_, theme.grid_minor_lines.r, theme.grid_minor_lines.g, theme.grid_minor_lines.b, theme.grid_minor_lines.a);
    }
    if (u_grid_major_color_ != -1) {
        glUniform4f(u_grid_major_color_, theme.grid_major_lines.r, theme.grid_major_lines.g, theme.grid_major_lines.b, theme.grid_major_lines.a);
    }
    if (u_grid_axis_x_color_ != -1) {
        glUniform4f(u_grid_axis_x_color_, theme.axis_x_color.r, theme.axis_x_color.g, theme.axis_x_color.b, 1.0f);
    }
    if (u_grid_axis_z_color_ != -1) {
        glUniform4f(u_grid_axis_z_color_, theme.axis_z_color.r, theme.axis_z_color.g, theme.axis_z_color.b, 1.0f);
    }
    if (u_line_size_ != -1) {
        glUniform1f(u_line_size_, line_width_);
    }
    if (u_grid_scale_ != -1) {
        glUniform1f(u_grid_scale_, grid_scale_);
    }
    if (u_grid_subdivision_ != -1) {
        glUniform1f(u_grid_subdivision_, grid_subdivision_);
    }
    
    // Render grid
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    glBindVertexArray(0);
    
    glUseProgram(0);
    glDisable(GL_BLEND);
}

// Color settings now come from CanvasTheme - no need for separate setters

void Grid3DRenderer::cleanup_opengl_resources() {
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (ebo_ != 0) {
        glDeleteBuffers(1, &ebo_);
        ebo_ = 0;
    }
    if (shader_program_ != 0) {
        glDeleteProgram(shader_program_);
        shader_program_ = 0;
    }
    if (vertex_shader_ != 0) {
        glDeleteShader(vertex_shader_);
        vertex_shader_ = 0;
    }
    if (fragment_shader_ != 0) {
        glDeleteShader(fragment_shader_);
        fragment_shader_ = 0;
    }
}

} // namespace voxel_canvas