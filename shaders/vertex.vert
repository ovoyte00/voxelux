#version 450

// Vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

// Uniform buffer object
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// Output to fragment shader
layout(location = 0) out vec3 fragColor;

void main() {
    // Transform vertex position
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    
    // Pass color to fragment shader
    fragColor = inColor;
}