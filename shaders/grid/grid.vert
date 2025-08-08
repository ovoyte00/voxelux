#version 410 core

// Vertex shader for infinite 3D grid (inspired by Blender's approach)
// Generates a full-screen quad that will procedurally generate the grid in fragment shader

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;
uniform vec3 camera_position;
uniform float grid_scale;

// Output to fragment shader
out vec3 world_pos;
out vec3 view_pos;
out vec2 screen_uv;

void main() {
    // Create a world space position for the grid plane
    // We'll create a large plane at Y=0 (XZ plane like Blender)
    vec3 world_position = position * grid_scale;
    world_position.y = 0.0; // Grid at Y=0 (ground plane)
    
    // Transform to view space
    vec4 view_position = view * vec4(world_position, 1.0);
    
    // Transform to clip space
    gl_Position = projection * view_position;
    
    // Pass world position to fragment shader
    world_pos = world_position;
    view_pos = view_position.xyz;
    screen_uv = texCoord;
}