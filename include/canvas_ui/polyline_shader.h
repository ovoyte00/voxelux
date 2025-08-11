/*
 * Copyright (C) 2024 Voxelux
 * 
 * Polyline Shader - Professional thick line rendering with geometry shader
 */

#pragma once

#include <vector>

namespace voxel_canvas {

struct LineVertex {
    float x, y, z;
    float r, g, b, a;
};

/**
 * Polyline shader that expands lines into thick quads using geometry shader
 * Modern GPU-accelerated polyline rendering with smooth color interpolation
 */
class PolylineShader {
public:
    PolylineShader();
    ~PolylineShader();
    
    bool initialize();
    void cleanup();
    
    void use();
    void unuse();
    
    // Set uniforms
    void set_projection(const float* matrix);
    void set_viewport_size(float width, float height);
    void set_line_width(float width);
    void set_line_smooth(bool smooth);
    
    // Draw functions
    void draw_line(float x1, float y1, float x2, float y2, 
                  float r, float g, float b, float a);
    void draw_lines(const std::vector<LineVertex>& lines);
    
    bool is_valid() const { return initialized_ && program_id_ != 0; }
    
private:
    unsigned int program_id_;
    unsigned int vertex_shader_;
    unsigned int geometry_shader_;
    unsigned int fragment_shader_;
    
    unsigned int vao_;
    unsigned int vbo_;
    
    // Uniform locations
    int u_projection_loc_;
    int u_viewport_size_loc_;
    int u_line_width_loc_;
    int u_line_smooth_loc_;
    
    bool initialized_;
};

} // namespace voxel_canvas