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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
    
    if (!create_grid_geometries()) {
        std::cerr << "ERROR: Failed to create grid geometries" << std::endl;
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
    u_grid_plane_ = glGetUniformLocation(shader_program_, "grid_plane");
    
    return true;
}

bool Grid3DRenderer::create_grid_geometries() {
    float size = grid_scale_;
    
    // XZ plane (horizontal floor grid, Y=0)
    float vertices_xz[] = {
        // Position (x, y, z), TexCoord (u, v)
        -size, 0.0f, -size,  0.0f, 0.0f,
         size, 0.0f, -size,  1.0f, 0.0f,
         size, 0.0f,  size,  1.0f, 1.0f,
        -size, 0.0f,  size,  0.0f, 1.0f
    };
    
    // YZ plane (vertical, at X=0)
    // This plane is perpendicular to the X axis
    float vertices_yz[] = {
        // Position (x, y, z), TexCoord (u, v)
        0.0f, -size, -size,  0.0f, 0.0f,  // bottom-left
        0.0f, -size,  size,  1.0f, 0.0f,  // bottom-right
        0.0f,  size,  size,  1.0f, 1.0f,  // top-right
        0.0f,  size, -size,  0.0f, 1.0f   // top-left
    };
    
    // XY plane (vertical, at Z=0)
    // This plane is perpendicular to the Z axis
    // Match the winding order of YZ plane which works
    float vertices_xy[] = {
        // Position (x, y, z), TexCoord (u, v)
        -size, -size, 0.0f,  0.0f, 0.0f,  // bottom-left
         size, -size, 0.0f,  1.0f, 0.0f,  // bottom-right
         size,  size, 0.0f,  1.0f, 1.0f,  // top-right
        -size,  size, 0.0f,  0.0f, 1.0f   // top-left
    };
    
    std::cout << "[Grid3D] Creating XY plane vertices at Z=0 with size " << (size*2) << "x" << (size*2) << std::endl;
    
    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };
    
    // Generate buffers
    glGenVertexArrays(1, &vao_xz_);
    glGenVertexArrays(1, &vao_yz_);
    glGenVertexArrays(1, &vao_xy_);
    glGenBuffers(1, &vbo_xz_);
    glGenBuffers(1, &vbo_yz_);
    glGenBuffers(1, &vbo_xy_);
    glGenBuffers(1, &ebo_);
    
    std::cout << "[Grid3D] Generated VAOs: XZ=" << vao_xz_ 
              << ", YZ=" << vao_yz_ 
              << ", XY=" << vao_xy_ << std::endl;
    
    // Upload index data (shared)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // Setup XZ plane (horizontal)
    glBindVertexArray(vao_xz_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_xz_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_xz), vertices_xz, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Setup YZ plane
    glBindVertexArray(vao_yz_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_yz_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_yz), vertices_yz, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Debug: Print the actual vertices being uploaded
    std::cout << "[Grid3D] YZ plane vertices:" << std::endl;
    for (int i = 0; i < 4; i++) {
        std::cout << "  Vertex " << i << ": (" 
                  << vertices_yz[i*5] << ", " 
                  << vertices_yz[i*5+1] << ", " 
                  << vertices_yz[i*5+2] << ")" << std::endl;
    }
    
    // Setup XY plane
    glBindVertexArray(vao_xy_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_xy_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_xy), vertices_xy, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Debug: Print the actual vertices being uploaded
    std::cout << "[Grid3D] XY plane vertices:" << std::endl;
    for (int i = 0; i < 4; i++) {
        std::cout << "  Vertex " << i << ": (" 
                  << vertices_xy[i*5] << ", " 
                  << vertices_xy[i*5+1] << ", " 
                  << vertices_xy[i*5+2] << ")" << std::endl;
    }
    
    // Debug: Check the buffer was uploaded
    GLint buffer_size = 0;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &buffer_size);
    std::cout << "[Grid3D] XY plane buffer size: " << buffer_size << " bytes (expected " << sizeof(vertices_xy) << ")" << std::endl;
    
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
    
    // Debug viewport
    static int debug_counter = 0;
    if (debug_counter++ % 60 == 0) {
        std::cout << "[Grid3D] Viewport: " << viewport.x << ", " << viewport.y 
                  << ", " << viewport.width << ", " << viewport.height << std::endl;
    }
    
    // Clear depth buffer before rendering grids
    glClear(GL_DEPTH_BUFFER_BIT);
    
    // Disable depth test for now to simplify debugging
    glDisable(GL_DEPTH_TEST);
    
    // Enable blending for grid transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Disable backface culling so grids are visible from both sides
    glDisable(GL_CULL_FACE);
    
    // Use grid shader
    glUseProgram(shader_program_);
    
    // Dynamic grid scaling based on camera distance - make it much larger
    float dynamic_grid_size = std::max(10000.0f, camera.get_distance() * 100.0f);
    
    // Set camera aspect ratio
    const_cast<Camera3D&>(camera).set_aspect_ratio(viewport.width / viewport.height);
    
    // Get matrices from professional Camera3D system
    Matrix4x4 view_matrix = camera.get_view_matrix();
    Matrix4x4 projection_matrix = camera.get_projection_matrix();
    
    // Debug projection matrix for vertical grids
    if (show_vertical_grid_ && debug_counter % 60 == 0) {
        std::cout << "[Grid3D] Projection matrix:" << std::endl;
        std::cout << "  [" << projection_matrix.m[0][0] << ", " << projection_matrix.m[0][1] << ", " << projection_matrix.m[0][2] << ", " << projection_matrix.m[0][3] << "]" << std::endl;
        std::cout << "  [" << projection_matrix.m[1][0] << ", " << projection_matrix.m[1][1] << ", " << projection_matrix.m[1][2] << ", " << projection_matrix.m[1][3] << "]" << std::endl;
        std::cout << "  [" << projection_matrix.m[2][0] << ", " << projection_matrix.m[2][1] << ", " << projection_matrix.m[2][2] << ", " << projection_matrix.m[2][3] << "]" << std::endl;
        std::cout << "  [" << projection_matrix.m[3][0] << ", " << projection_matrix.m[3][1] << ", " << projection_matrix.m[3][2] << ", " << projection_matrix.m[3][3] << "]" << std::endl;
    }
    
    // Debug view matrix
    if (debug_counter % 60 == 0) {
        std::cout << "[Grid3D] View matrix before upload:" << std::endl;
        std::cout << "  [" << view_matrix.m[0][0] << ", " << view_matrix.m[0][1] << ", " << view_matrix.m[0][2] << ", " << view_matrix.m[0][3] << "]" << std::endl;
        std::cout << "  [" << view_matrix.m[1][0] << ", " << view_matrix.m[1][1] << ", " << view_matrix.m[1][2] << ", " << view_matrix.m[1][3] << "]" << std::endl;
        std::cout << "  [" << view_matrix.m[2][0] << ", " << view_matrix.m[2][1] << ", " << view_matrix.m[2][2] << ", " << view_matrix.m[2][3] << "]" << std::endl;
        std::cout << "  [" << view_matrix.m[3][0] << ", " << view_matrix.m[3][1] << ", " << view_matrix.m[3][2] << ", " << view_matrix.m[3][3] << "]" << std::endl;
    }
    
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
    
    // Render vertical grid FIRST if enabled (so horizontal grid doesn't cover it)
    if (show_vertical_grid_) {
        // Only log once per second to avoid spam
        static int frame_counter = 0;
        if (frame_counter++ % 60 == 0) {
            std::cout << "[Grid3D] Rendering vertical grid, active_plane=" << active_plane_ << std::endl;
        }
        
        // Render the appropriate vertical plane based on view
        Matrix4x4 vertical_model = Matrix4x4::identity();
        GLuint vao_to_use = 0;
        
        if (active_plane_ == PLANE_YZ) {
            float scale = dynamic_grid_size/grid_scale_;
            if (frame_counter % 60 == 1) {
                std::cout << "[Grid3D] YZ plane scale=" << scale << ", dynamic_grid_size=" << dynamic_grid_size << std::endl;
            }
            // YZ plane (for X-axis views) - scale Y and Z, keep X at 1
            vertical_model = Matrix4x4::scale({1.0f, scale, scale});
            vao_to_use = vao_yz_;
        } else if (active_plane_ == PLANE_XY) {
            // Try the EXACT same scaling as YZ plane which works
            float scale = dynamic_grid_size/grid_scale_;
            if (frame_counter % 60 == 1) {
                std::cout << "[Grid3D] XY plane scale=" << scale << ", dynamic_grid_size=" << dynamic_grid_size << std::endl;
                Vector3D cam_pos = camera.get_position();
                std::cout << "[Grid3D] Camera position: " << cam_pos.x << ", " << cam_pos.y << ", " << cam_pos.z << std::endl;
                std::cout << "[Grid3D] XY VAO = " << vao_xy_ << " (should be 6)" << std::endl;
            }
            // XY plane (for Z-axis views) - scale X and Y, keep Z at 1
            // EXACTLY like YZ does it
            vertical_model = Matrix4x4::scale({scale, scale, 1.0f});
            vao_to_use = vao_xy_;
            
            // Debug: check if VAO is valid
            if (!glIsVertexArray(vao_xy_)) {
                std::cout << "[Grid3D] ERROR: XY VAO " << vao_xy_ << " is not valid!" << std::endl;
            }
        }
        
        // Update uniforms and render
        glUniformMatrix4fv(u_model_, 1, GL_TRUE, vertical_model.data());
        if (u_grid_plane_ != -1) {
            glUniform1i(u_grid_plane_, active_plane_);
        }
        
        if (frame_counter % 60 == 1) {
            std::cout << "[Grid3D] Using model matrix with scale " << dynamic_grid_size/grid_scale_ << std::endl;
        }
        
        // Render as backdrop - disable depth test entirely
        glDisable(GL_DEPTH_TEST);
        
        // DEBUG: Also disable face culling in case winding is wrong
        glDisable(GL_CULL_FACE);
        
        // DEBUG: Make sure blending is correct
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Debug: Check if VAO is valid
        if (vao_to_use == 0) {
            std::cout << "[Grid3D] ERROR: VAO is 0!" << std::endl;
        } else if (frame_counter % 60 == 1) {
            std::cout << "[Grid3D] Using VAO " << vao_to_use << " for plane " << active_plane_ << std::endl;
        }
        
        glBindVertexArray(vao_to_use);
        
        if (frame_counter % 60 == 1) {
            std::cout << "[Grid3D] About to draw vertical grid with VAO " << vao_to_use 
                      << " (XZ=" << vao_xz_ << ", YZ=" << vao_yz_ << ", XY=" << vao_xy_ << ")" << std::endl;
        }
        
        // DEBUG: Check if shader program is valid
        if (frame_counter % 60 == 1) {
            GLint is_linked = 0;
            glGetProgramiv(shader_program_, GL_LINK_STATUS, &is_linked);
            std::cout << "[Grid3D] Shader program " << shader_program_ 
                      << " link status: " << is_linked << std::endl;
                      
            // Check uniform locations
            std::cout << "[Grid3D] Uniform locations: model=" << u_model_
                      << ", view=" << u_view_
                      << ", proj=" << u_projection_
                      << ", grid_plane=" << u_grid_plane_ << std::endl;
                      
            // Also check the matrices
            float view_matrix[16];
            glGetUniformfv(shader_program_, u_view_, view_matrix);
            std::cout << "[Grid3D] View matrix [0][0]=" << view_matrix[0] 
                      << ", [1][1]=" << view_matrix[5]
                      << ", [2][2]=" << view_matrix[10] 
                      << ", [3][2]=" << view_matrix[14] << " (Z translation)" << std::endl;
        }
        
        // Debug for XY plane specifically
        if (active_plane_ == PLANE_XY) {
            std::cout << "[Grid3D] Drawing XY plane with VAO " << vao_to_use << std::endl;
            
            // Check if we have a valid VAO bound
            GLint current_vao = 0;
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &current_vao);
            std::cout << "[Grid3D] Current VAO bound: " << current_vao << std::endl;
            
            // Check the model matrix we're sending
            std::cout << "[Grid3D] Model matrix for XY plane:" << std::endl;
            std::cout << "  [" << vertical_model.m[0][0] << ", " << vertical_model.m[0][1] << ", " << vertical_model.m[0][2] << ", " << vertical_model.m[0][3] << "]" << std::endl;
            std::cout << "  [" << vertical_model.m[1][0] << ", " << vertical_model.m[1][1] << ", " << vertical_model.m[1][2] << ", " << vertical_model.m[1][3] << "]" << std::endl;
            std::cout << "  [" << vertical_model.m[2][0] << ", " << vertical_model.m[2][1] << ", " << vertical_model.m[2][2] << ", " << vertical_model.m[2][3] << "]" << std::endl;
            std::cout << "  [" << vertical_model.m[3][0] << ", " << vertical_model.m[3][1] << ", " << vertical_model.m[3][2] << ", " << vertical_model.m[3][3] << "]" << std::endl;
            
            // The vertices after transform should be visible
            float scale = dynamic_grid_size/grid_scale_;
            std::cout << "[Grid3D] XY vertices after scale " << scale << ":" << std::endl;
            std::cout << "  Min: (" << (-grid_scale_ * scale) << ", " << (-grid_scale_ * scale) << ", 0)" << std::endl;
            std::cout << "  Max: (" << (grid_scale_ * scale) << ", " << (grid_scale_ * scale) << ", 0)" << std::endl;
        }
        
        // Extra debug for XY plane
        if (active_plane_ == PLANE_XY) {
            // Check if we have vertices
            GLint vbo_size = 0;
            glBindBuffer(GL_ARRAY_BUFFER, vbo_xy_);
            glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &vbo_size);
            std::cout << "[Grid3D] VBO size for XY: " << vbo_size << " bytes" << std::endl;
            
            // Check element buffer
            GLint ebo_size = 0;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
            glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &ebo_size);
            std::cout << "[Grid3D] EBO size: " << ebo_size << " bytes" << std::endl;
            
            // Check if shader uniform is set correctly
            GLint plane_value = -1;
            glGetUniformiv(shader_program_, u_grid_plane_, &plane_value);
            std::cout << "[Grid3D] grid_plane uniform value: " << plane_value << " (should be 2)" << std::endl;
        }
        
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        // Check for errors after draw for XY plane
        if (active_plane_ == PLANE_XY) {
            GLenum draw_err = glGetError();
            if (draw_err != GL_NO_ERROR) {
                std::cout << "[Grid3D] OpenGL Error after XY draw: " << draw_err << std::endl;
            } else {
                std::cout << "[Grid3D] XY draw completed without errors" << std::endl;
            }
        }
        
        // Debug: Check what we actually rendered
        if (frame_counter % 60 == 1) {
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            std::cout << "[Grid3D] Drew vertical grid. Viewport: " << viewport[0] << "," << viewport[1] 
                      << "," << viewport[2] << "," << viewport[3] << std::endl;
        }
        
        // Check for OpenGL errors
        GLenum err = glGetError();
        if (err != GL_NO_ERROR && frame_counter % 60 == 1) {
            std::cout << "[Grid3D] OpenGL Error after draw: " << err << std::endl;
        }
        
        glEnable(GL_DEPTH_TEST);  // Re-enable depth testing
    }
    
    // Always render horizontal grid (after vertical grid so it doesn't cover it)
    if (u_grid_plane_ != -1) {
        glUniform1i(u_grid_plane_, PLANE_XZ);
    }
    
    // Reset model matrix for horizontal grid
    Matrix4x4 horizontal_model = Matrix4x4::scale({dynamic_grid_size/grid_scale_, 1.0f, dynamic_grid_size/grid_scale_});
    glUniformMatrix4fv(u_model_, 1, GL_TRUE, horizontal_model.data());
    
    glBindVertexArray(vao_xz_);
    
    // Check for errors before drawing
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cout << "[Grid3D] OpenGL Error before horizontal grid draw: " << err << std::endl;
    }
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cout << "[Grid3D] OpenGL Error after horizontal grid draw: " << err << std::endl;
    }
    
    glBindVertexArray(0);
    
    glUseProgram(0);
    glDisable(GL_BLEND);
}

// Color settings now come from CanvasTheme - no need for separate setters

void Grid3DRenderer::cleanup_opengl_resources() {
    if (vao_xz_ != 0) {
        glDeleteVertexArrays(1, &vao_xz_);
        vao_xz_ = 0;
    }
    if (vao_yz_ != 0) {
        glDeleteVertexArrays(1, &vao_yz_);
        vao_yz_ = 0;
    }
    if (vao_xy_ != 0) {
        glDeleteVertexArrays(1, &vao_xy_);
        vao_xy_ = 0;
    }
    if (vbo_xz_ != 0) {
        glDeleteBuffers(1, &vbo_xz_);
        vbo_xz_ = 0;
    }
    if (vbo_yz_ != 0) {
        glDeleteBuffers(1, &vbo_yz_);
        vbo_yz_ = 0;
    }
    if (vbo_xy_ != 0) {
        glDeleteBuffers(1, &vbo_xy_);
        vbo_xy_ = 0;
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