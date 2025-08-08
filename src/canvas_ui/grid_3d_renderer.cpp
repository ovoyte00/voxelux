/*
 * Copyright (C) 2024 Voxelux
 * 
 * Professional 3D Grid Renderer implementation
 */

#include "canvas_ui/grid_3d_renderer.h"
#include "glad/glad.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

namespace voxel_canvas {

// Camera3D implementation

void Camera3D::get_view_matrix(float* matrix) const {
    // Create view matrix from camera state
    float pitch_rad = rotation.x * M_PI / 180.0f;
    float yaw_rad = rotation.y * M_PI / 180.0f;
    
    // Calculate camera position from spherical coordinates
    float cam_x = distance * cos(pitch_rad) * cos(yaw_rad);
    float cam_y = distance * sin(pitch_rad);
    float cam_z = distance * cos(pitch_rad) * sin(yaw_rad);
    
    // Camera position
    float eye_x = target.x + cam_x;
    float eye_y = cam_y;
    float eye_z = target.y + cam_z;
    
    // Look at target
    float center_x = target.x;
    float center_y = 0.0f;
    float center_z = target.y;
    
    // Up vector
    float up_x = 0.0f, up_y = 1.0f, up_z = 0.0f;
    
    // Create look-at matrix
    float forward_x = center_x - eye_x;
    float forward_y = center_y - eye_y;
    float forward_z = center_z - eye_z;
    float forward_len = sqrt(forward_x*forward_x + forward_y*forward_y + forward_z*forward_z);
    forward_x /= forward_len;
    forward_y /= forward_len;
    forward_z /= forward_len;
    
    float right_x = forward_y * up_z - forward_z * up_y;
    float right_y = forward_z * up_x - forward_x * up_z;
    float right_z = forward_x * up_y - forward_y * up_x;
    float right_len = sqrt(right_x*right_x + right_y*right_y + right_z*right_z);
    right_x /= right_len;
    right_y /= right_len;
    right_z /= right_len;
    
    float up_new_x = right_y * forward_z - right_z * forward_y;
    float up_new_y = right_z * forward_x - right_x * forward_z;
    float up_new_z = right_x * forward_y - right_y * forward_x;
    
    // Build view matrix (column-major for OpenGL)
    matrix[0] = right_x;
    matrix[1] = up_new_x;
    matrix[2] = -forward_x;
    matrix[3] = 0.0f;
    
    matrix[4] = right_y;
    matrix[5] = up_new_y;
    matrix[6] = -forward_y;
    matrix[7] = 0.0f;
    
    matrix[8] = right_z;
    matrix[9] = up_new_z;
    matrix[10] = -forward_z;
    matrix[11] = 0.0f;
    
    matrix[12] = -(right_x * eye_x + right_y * eye_y + right_z * eye_z);
    matrix[13] = -(up_new_x * eye_x + up_new_y * eye_y + up_new_z * eye_z);
    matrix[14] = -(-forward_x * eye_x + -forward_y * eye_y + -forward_z * eye_z);
    matrix[15] = 1.0f;
}

void Camera3D::get_projection_matrix(float* matrix, float aspect_ratio, float fov) const {
    if (orthographic) {
        // Orthographic projection
        float left = -ortho_scale * aspect_ratio;
        float right = ortho_scale * aspect_ratio;
        float bottom = -ortho_scale;
        float top = ortho_scale;
        float near_plane = 0.1f;
        float far_plane = 1000.0f;
        
        matrix[0] = 2.0f / (right - left);
        matrix[1] = 0.0f;
        matrix[2] = 0.0f;
        matrix[3] = 0.0f;
        
        matrix[4] = 0.0f;
        matrix[5] = 2.0f / (top - bottom);
        matrix[6] = 0.0f;
        matrix[7] = 0.0f;
        
        matrix[8] = 0.0f;
        matrix[9] = 0.0f;
        matrix[10] = -2.0f / (far_plane - near_plane);
        matrix[11] = 0.0f;
        
        matrix[12] = -(right + left) / (right - left);
        matrix[13] = -(top + bottom) / (top - bottom);
        matrix[14] = -(far_plane + near_plane) / (far_plane - near_plane);
        matrix[15] = 1.0f;
    } else {
        // Perspective projection
        float fov_rad = fov * M_PI / 180.0f;
        float near_plane = 0.1f;
        float far_plane = 1000.0f;
        
        float f = 1.0f / tan(fov_rad * 0.5f);
        
        matrix[0] = f / aspect_ratio;
        matrix[1] = 0.0f;
        matrix[2] = 0.0f;
        matrix[3] = 0.0f;
        
        matrix[4] = 0.0f;
        matrix[5] = f;
        matrix[6] = 0.0f;
        matrix[7] = 0.0f;
        
        matrix[8] = 0.0f;
        matrix[9] = 0.0f;
        matrix[10] = (far_plane + near_plane) / (near_plane - far_plane);
        matrix[11] = -1.0f;
        
        matrix[12] = 0.0f;
        matrix[13] = 0.0f;
        matrix[14] = (2.0f * far_plane * near_plane) / (near_plane - far_plane);
        matrix[15] = 0.0f;
    }
}

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
        std::cerr << "Failed to load grid shaders" << std::endl;
        return false;
    }
    
    if (!create_grid_geometry()) {
        std::cerr << "Failed to create grid geometry" << std::endl;
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
    // Load vertex shader
    std::ifstream vertex_file("shaders/grid/grid.vert");
    if (!vertex_file.is_open()) {
        std::cerr << "Failed to open vertex shader file" << std::endl;
        return false;
    }
    
    std::stringstream vertex_stream;
    vertex_stream << vertex_file.rdbuf();
    std::string vertex_code = vertex_stream.str();
    vertex_file.close();
    
    // Load fragment shader
    std::ifstream fragment_file("shaders/grid/grid.frag");
    if (!fragment_file.is_open()) {
        std::cerr << "Failed to open fragment shader file" << std::endl;
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
    u_grid_emphasis_color_ = glGetUniformLocation(shader_program_, "grid_emphasis_color");
    u_grid_axis_x_color_ = glGetUniformLocation(shader_program_, "grid_axis_x_color");
    u_grid_axis_z_color_ = glGetUniformLocation(shader_program_, "grid_axis_z_color");
    u_line_size_ = glGetUniformLocation(shader_program_, "line_size");
    u_grid_scale_ = glGetUniformLocation(shader_program_, "grid_scale");
    
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
    
    // Enable blending for grid transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Use grid shader
    glUseProgram(shader_program_);
    
    // Dynamic grid scaling based on camera distance (like archived implementation)
    float dynamic_grid_size = std::max(2000.0f, camera.distance * 6.0f); // Huge grid coverage for far view
    
    // Set matrices
    float view_matrix[16];
    float projection_matrix[16];
    float model_matrix[16] = {
        dynamic_grid_size/grid_scale_, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, dynamic_grid_size/grid_scale_, 0,
        0, 0, 0, 1
    };
    
    camera.get_view_matrix(view_matrix);
    camera.get_projection_matrix(projection_matrix, viewport.width / viewport.height);
    
    glUniformMatrix4fv(u_view_, 1, GL_FALSE, view_matrix);
    glUniformMatrix4fv(u_projection_, 1, GL_FALSE, projection_matrix);
    glUniformMatrix4fv(u_model_, 1, GL_FALSE, model_matrix);
    
    // Set camera uniforms
    if (u_camera_distance_ != -1) {
        glUniform1f(u_camera_distance_, camera.distance);
    }
    
    // Set grid appearance uniforms using theme colors (using glUniform4f for cross-platform compatibility)
    if (u_grid_color_ != -1) {
        glUniform4f(u_grid_color_, theme.grid_minor_lines.r, theme.grid_minor_lines.g, theme.grid_minor_lines.b, 1.0f);
    }
    if (u_grid_emphasis_color_ != -1) {
        glUniform4f(u_grid_emphasis_color_, theme.grid_major_lines.r, theme.grid_major_lines.g, theme.grid_major_lines.b, 1.0f);
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