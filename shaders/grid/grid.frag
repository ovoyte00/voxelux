#version 410 core

// Fragment shader for infinite 3D grid with hierarchical detail levels
// Voxel-based grid with hierarchical detail rendering

in vec3 world_pos;
in vec3 view_pos;
in vec2 screen_uv;

uniform vec3 camera_position;
uniform float camera_distance;
uniform vec4 grid_color;
uniform vec4 grid_major_color;
uniform vec4 grid_axis_x_color;
uniform vec4 grid_axis_z_color;
uniform float line_size;
uniform float grid_scale;
uniform float grid_subdivision; // Subdivision factor (10 for decimal, 16 for Minecraft, etc.)

out vec4 color;

// Professional line rendering with anti-aliasing
#define DISC_RADIUS 0.59
#define LINE_SMOOTH_START (0.5 - DISC_RADIUS) 
#define LINE_SMOOTH_END (0.5 + DISC_RADIUS)
#define LINE_STEP(dist) smoothstep(LINE_SMOOTH_START, LINE_SMOOTH_END, dist)

// Get grid line intensity at a given position with anti-aliasing
float get_grid_line(vec2 pos, vec2 fwidth_pos, float grid_step) {
    // Calculate distance to nearest grid lines
    vec2 grid_pos = abs(mod(pos + grid_step * 0.5, grid_step) - grid_step * 0.5);
    
    // Modulate by derivative for consistent line width under perspective
    vec2 grid_domain = grid_pos / fwidth_pos;
    
    // Collapse to single line distance
    float line_dist = min(grid_domain.x, grid_domain.y);
    
    // Return line intensity using smooth step function
    return 1.0 - LINE_STEP(line_dist - line_size);
}

// Get axis line intensity
float get_axis_line(vec3 co, vec3 fwidth_pos, float axis_size) {
    vec3 axes_domain = abs(co) / fwidth_pos;
    // Axes use same thickness as grid lines
    return 1.0 - LINE_STEP(min(min(axes_domain.x, axes_domain.y), axes_domain.z) - line_size);
}

// Linear step for smooth transitions
float linearstep(float edge0, float edge1, float x) {
    return clamp((x - edge0) / abs(edge1 - edge0), 0.0, 1.0);
}

void main() {
    // Calculate derivatives for anti-aliasing (fwidth)
    vec3 P = world_pos;
    vec3 dFdxPos = dFdx(P);
    vec3 dFdyPos = dFdy(P);
    vec3 fwidthPos = abs(dFdxPos) + abs(dFdyPos);
    
    // Grid position and derivatives on XZ plane
    vec2 grid_pos = P.xz;
    vec2 grid_fwidth = fwidthPos.xz;
    
    // Calculate distance for overall fading
    vec3 V = camera_position - P;
    float dist = length(V);
    V = V / dist;  // Normalize
    
    // Angle-based fading (fade when looking parallel to grid)
    float angle = 1.0 - abs(V.y);
    angle = angle * angle;
    float angle_fade = 1.0 - angle * angle;
    
    // Distance-based fading for horizon
    float fade_start = 200.0;
    float fade_end = 1000.0;
    float distance_fade = 1.0 - smoothstep(fade_start, fade_end, dist);
    
    // Combined fade for the entire grid
    float fade = angle_fade * distance_fade;
    
    // Grid resolution using screen-space derivatives
    // This tells us how much world space is covered by one pixel at this fragment
    // Using max of X and Z derivatives for consistent grid appearance
    float grid_res = max(abs(dFdxPos.x), abs(dFdxPos.z));
    
    // Grid appears when it's about 4 pixels wide
    grid_res *= 4.0;
    
    // Grid subdivision (default 10, can be 16 for Minecraft)
    float subdiv = grid_subdivision > 0.0 ? grid_subdivision : 10.0;
    
    // Grid scale steps - powers of subdivision
    const int STEPS_LEN = 8;
    float steps[STEPS_LEN];
    steps[0] = 1.0;  // Base: 1 voxel
    for (int i = 1; i < STEPS_LEN; i++) {
        steps[i] = steps[i-1] * subdiv;
    }
    
    // Find appropriate scale level based on grid resolution
    int step_id = STEPS_LEN - 1;
    for (int i = STEPS_LEN - 2; i >= 0; i--) {
        if (grid_res < steps[i]) {
            step_id = i;
        }
    }
    
    // Select three consecutive scales for smooth transitions
    float scale0 = step_id > 0 ? steps[step_id - 1] : 0.0;
    float scaleA = steps[step_id];
    float scaleB = min(step_id + 1, STEPS_LEN - 1) < STEPS_LEN ? steps[min(step_id + 1, STEPS_LEN - 1)] : scaleA;
    float scaleC = min(step_id + 2, STEPS_LEN - 1) < STEPS_LEN ? steps[min(step_id + 2, STEPS_LEN - 1)] : scaleB;
    
    // Calculate blend factor for smooth scale transitions
    // Smooth transition from previous scale to current scale
    float blend = 1.0 - linearstep(scale0, scaleA, grid_res);
    blend = blend * blend * blend; // Cubic for smooth transition
    
    // Get grid lines for each scale
    float gridA = get_grid_line(grid_pos, grid_fwidth, scaleA);
    float gridB = get_grid_line(grid_pos, grid_fwidth, scaleB);
    float gridC = get_grid_line(grid_pos, grid_fwidth, scaleC);
    
    // Combine grids with proper blending
    vec4 out_color = grid_color;
    out_color.a *= gridA * blend; // Finest grid fades out uniformly via blend
    
    // Mix in medium scale (transitions from minor to major color based on blend)
    out_color = mix(out_color, mix(grid_color, grid_major_color, blend), gridB);
    
    // Mix in coarsest scale (always major color)
    out_color = mix(out_color, grid_major_color, gridC);
    
    // Axis lines - make them more visible with a larger threshold
    float axis_threshold = max(2.0, line_size * 2.0);
    if (abs(P.z) < fwidthPos.z * axis_threshold) {
        // X-axis (red)
        float x_axis = 1.0 - LINE_STEP(abs(P.z) / fwidthPos.z - line_size);
        out_color = mix(out_color, grid_axis_x_color, x_axis);
    }
    if (abs(P.x) < fwidthPos.x * axis_threshold) {
        // Z-axis (blue)  
        float z_axis = 1.0 - LINE_STEP(abs(P.x) / fwidthPos.x - line_size);
        out_color = mix(out_color, grid_axis_z_color, z_axis);
    }
    
    // Apply overall fade
    out_color.a *= fade;
    
    color = out_color;
}