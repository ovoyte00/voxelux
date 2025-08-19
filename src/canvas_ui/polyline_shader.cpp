/*
 * Copyright (C) 2024 Voxelux
 * 
 * Polyline Shader - Thick line rendering with geometry shader
 */

#include "canvas_ui/polyline_shader.h"
#include "glad/gl.h"
#include <iostream>
#include <vector>

namespace voxel_canvas {

// Vertex shader that passes through line endpoints
static const char* polyline_vertex_shader = R"(
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

out VS_OUT {
    vec4 color;
} vs_out;

uniform mat4 u_projection;

void main() {
    gl_Position = u_projection * vec4(position.xy, 0.0, 1.0);
    vs_out.color = color;
}
)";

// Geometry shader that expands lines into quads
static const char* polyline_geometry_shader = R"(
#version 330 core

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

in VS_OUT {
    vec4 color;
} gs_in[];

out vec4 frag_color;
out float v_line_coord;  // For anti-aliasing: -1 at one edge, +1 at other edge

uniform vec2 u_viewport_size;
uniform float u_line_width;
uniform bool u_line_smooth;

void main() {
    vec4 p0 = gl_in[0].gl_Position;
    vec4 p1 = gl_in[1].gl_Position;
    
    // Convert to screen space
    vec2 screen0 = (p0.xy / p0.w) * u_viewport_size * 0.5;
    vec2 screen1 = (p1.xy / p1.w) * u_viewport_size * 0.5;
    
    // Calculate line direction and perpendicular
    vec2 line_dir = normalize(screen1 - screen0);
    vec2 line_normal = vec2(-line_dir.y, line_dir.x);
    
    // Convert back to NDC space
    vec2 offset = line_normal * u_line_width / u_viewport_size;
    
    // Emit quad vertices
    // First vertex (start - offset)
    gl_Position = p0;
    gl_Position.xy -= offset * p0.w;
    frag_color = gs_in[0].color;
    v_line_coord = -1.0;
    EmitVertex();
    
    // Second vertex (start + offset)
    gl_Position = p0;
    gl_Position.xy += offset * p0.w;
    frag_color = gs_in[0].color;
    v_line_coord = 1.0;
    EmitVertex();
    
    // Third vertex (end - offset)
    gl_Position = p1;
    gl_Position.xy -= offset * p1.w;
    frag_color = gs_in[1].color;
    v_line_coord = -1.0;
    EmitVertex();
    
    // Fourth vertex (end + offset)
    gl_Position = p1;
    gl_Position.xy += offset * p1.w;
    frag_color = gs_in[1].color;
    v_line_coord = 1.0;
    EmitVertex();
    
    EndPrimitive();
}
)";

// Fragment shader with anti-aliasing
static const char* polyline_fragment_shader = R"(
#version 330 core

in vec4 frag_color;
in float v_line_coord;

out vec4 out_color;

uniform bool u_line_smooth;
uniform float u_line_width;

void main() {
    out_color = frag_color;
    
    // Anti-aliasing for smooth lines
    if (u_line_smooth) {
        float dist = abs(v_line_coord);
        float edge_fade = 1.0 - smoothstep(0.5, 1.0, dist);
        out_color.a *= edge_fade;
    }
}
)";

PolylineShader::PolylineShader() 
    : program_id_(0)
    , vertex_shader_(0)
    , geometry_shader_(0)
    , fragment_shader_(0)
    , vao_(0)
    , vbo_(0)
    , initialized_(false) {
}

PolylineShader::~PolylineShader() {
    cleanup();
}

bool PolylineShader::initialize() {
    if (initialized_) {
        return true;
    }
    
    // Compile vertex shader
    vertex_shader_ = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader_, 1, &polyline_vertex_shader, nullptr);
    glCompileShader(vertex_shader_);
    
    GLint success;
    GLchar info_log[512];
    glGetShaderiv(vertex_shader_, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex_shader_, 512, nullptr, info_log);
        std::cerr << "Polyline vertex shader compilation failed: " << info_log << std::endl;
        return false;
    }
    
    // Compile geometry shader
    geometry_shader_ = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(geometry_shader_, 1, &polyline_geometry_shader, nullptr);
    glCompileShader(geometry_shader_);
    
    glGetShaderiv(geometry_shader_, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(geometry_shader_, 512, nullptr, info_log);
        std::cerr << "Polyline geometry shader compilation failed: " << info_log << std::endl;
        return false;
    }
    
    // Compile fragment shader
    fragment_shader_ = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader_, 1, &polyline_fragment_shader, nullptr);
    glCompileShader(fragment_shader_);
    
    glGetShaderiv(fragment_shader_, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment_shader_, 512, nullptr, info_log);
        std::cerr << "Polyline fragment shader compilation failed: " << info_log << std::endl;
        return false;
    }
    
    // Link program
    program_id_ = glCreateProgram();
    glAttachShader(program_id_, vertex_shader_);
    glAttachShader(program_id_, geometry_shader_);
    glAttachShader(program_id_, fragment_shader_);
    glLinkProgram(program_id_);
    
    glGetProgramiv(program_id_, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program_id_, 512, nullptr, info_log);
        std::cerr << "Polyline shader linking failed: " << info_log << std::endl;
        return false;
    }
    
    // Get uniform locations
    u_projection_loc_ = glGetUniformLocation(program_id_, "u_projection");
    u_viewport_size_loc_ = glGetUniformLocation(program_id_, "u_viewport_size");
    u_line_width_loc_ = glGetUniformLocation(program_id_, "u_line_width");
    u_line_smooth_loc_ = glGetUniformLocation(program_id_, "u_line_smooth");
    
    // Create VAO and VBO
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    
    // Setup vertex attributes: position (3 floats) + color (4 floats)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    
    glBindVertexArray(0);
    
    initialized_ = true;
    return true;
}

void PolylineShader::cleanup() {
    if (vao_) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (vbo_) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (program_id_) {
        glDeleteProgram(program_id_);
        program_id_ = 0;
    }
    if (vertex_shader_) {
        glDeleteShader(vertex_shader_);
        vertex_shader_ = 0;
    }
    if (geometry_shader_) {
        glDeleteShader(geometry_shader_);
        geometry_shader_ = 0;
    }
    if (fragment_shader_) {
        glDeleteShader(fragment_shader_);
        fragment_shader_ = 0;
    }
    initialized_ = false;
}

void PolylineShader::use() {
    if (program_id_) {
        glUseProgram(program_id_);
    }
}

void PolylineShader::unuse() {
    glUseProgram(0);
}

void PolylineShader::set_projection(const float* matrix) {
    glUniformMatrix4fv(u_projection_loc_, 1, GL_FALSE, matrix);
}

void PolylineShader::set_viewport_size(float width, float height) {
    glUniform2f(u_viewport_size_loc_, width, height);
}

void PolylineShader::set_line_width(float width) {
    glUniform1f(u_line_width_loc_, width);
}

void PolylineShader::set_line_smooth(bool smooth) {
    glUniform1i(u_line_smooth_loc_, smooth ? 1 : 0);
}

void PolylineShader::draw_line(float x1, float y1, float x2, float y2, 
                               float r, float g, float b, float a) {
    // Create line vertices
    float vertices[] = {
        x1, y1, 0.0f, r, g, b, a,  // Start point
        x2, y2, 0.0f, r, g, b, a   // End point
    };
    
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    
    glDrawArrays(GL_LINES, 0, 2);
    
    glBindVertexArray(0);
}

void PolylineShader::draw_lines(const std::vector<LineVertex>& lines) {
    if (lines.empty()) return;
    
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(lines.size() * sizeof(LineVertex)), lines.data(), GL_DYNAMIC_DRAW);
    
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lines.size()));
    
    glBindVertexArray(0);
}

} // namespace voxel_canvas