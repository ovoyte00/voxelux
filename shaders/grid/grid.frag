#version 410 core

// Fragment shader for infinite 3D grid (inspired by Blender's approach)
// Procedurally generates grid lines with anti-aliasing and distance-based fading

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

out vec4 color;

// Get grid intensity at a given position with anti-aliasing
float get_grid_line(vec2 pos, vec2 derivative, float grid_step) {
    vec2 grid_pos = abs(mod(pos + grid_step * 0.5, grid_step) - grid_step * 0.5);
    vec2 derivative_scaled = derivative / grid_step;
    vec2 grid_smooth = grid_pos / derivative_scaled;
    float line_dist = min(grid_smooth.x, grid_smooth.y);
    return 1.0 - smoothstep(line_size - 0.5, line_size + 0.5, line_dist);
}

// Get axis line intensity
float get_axis_line(float pos, float derivative, float line_width) {
    float axis_dist = abs(pos) / derivative;
    return 1.0 - smoothstep(line_width - 0.5, line_width + 0.5, axis_dist);
}

void main() {
    // Calculate derivatives for anti-aliasing
    vec2 grid_pos = world_pos.xz;
    vec2 derivative = abs(dFdx(grid_pos)) + abs(dFdy(grid_pos));
    
    // Distance-based fading
    float distance_to_camera = length(view_pos);
    
    // Professional angle-based fading for grid visibility
    vec3 view_dir = normalize(view_pos);
    float angle = 1.0 - abs(view_dir.y); // Fade when looking parallel to grid
    angle = angle * angle; // Smooth transition curve
    float angle_fade = 1.0 - angle * angle; // Standard double square fade
    
    // Distance-based grid fade for depth perception
    float fade_start = camera_distance * 50.0; // Begin fade distance
    float fade_end = camera_distance * 100.0;   // Complete fade distance
    float distance_fade = 1.0 - smoothstep(fade_start, fade_end, distance_to_camera);
    
    vec3 final_color = vec3(0.0);
    float final_alpha = 0.0;
    
    // Multiple grid scales for detail levels
    float scaleA = 1.0;   // Fine grid
    float scaleB = 10.0;  // Medium grid  
    float scaleC = 100.0; // Coarse grid
    
    // Calculate grid resolution for level-of-detail blending
    float grid_res = length(derivative);
    
    // Multi-level grid system with level-of-detail fading
    float blendA = 1.0 - smoothstep(scaleA * 0.5, scaleA * 4.0, grid_res);
    float blendB = 1.0 - smoothstep(scaleB * 0.5, scaleB * 4.0, grid_res);
    float blendC = 1.0 - smoothstep(scaleC * 0.5, scaleC * 4.0, grid_res);
    
    // Apply cubic blending for smooth transitions
    blendA = blendA * blendA * blendA;
    blendB = blendB * blendB * blendB;
    blendC = blendC * blendC * blendC;
    
    float gridA = get_grid_line(grid_pos, derivative, scaleA);
    float gridB = get_grid_line(grid_pos, derivative, scaleB);
    float gridC = get_grid_line(grid_pos, derivative, scaleC);
    
    // Grid hierarchy using color mixing approach (like Blender)
    // Base grid uses regular color, emphasis levels use emphasis color
    float gridAlpha = 0.0;
    vec3 gridColor = grid_color.rgb;
    
    // Fine grid (1 unit) - base color with base alpha
    float baseAlpha = grid_color.a;  // Use alpha from grid_color uniform
    float majorAlpha = grid_major_color.a;  // Use alpha from major grid color
    
    if (blendA > 0.01) {
        float a1 = gridA * blendA * baseAlpha;
        gridAlpha = a1;
    }
    
    // Medium grid (10 units) - mix towards major color and alpha
    if (blendB > 0.01) {
        float a10 = gridB * blendB;
        float mixedAlpha = mix(baseAlpha, majorAlpha, blendB) * a10;
        if (mixedAlpha > gridAlpha) {
            gridColor = mix(grid_color.rgb, grid_major_color.rgb, blendB);
            gridAlpha = mixedAlpha;
        }
    }
    
    // Coarse grid (100 units) - full major color and alpha
    if (blendC > 0.01) {
        float a100 = gridC * blendC * majorAlpha;
        if (a100 > gridAlpha) {
            gridColor = grid_major_color.rgb;
            gridAlpha = a100;
        }
    }
    
    // Apply distance and angle fading
    gridAlpha *= distance_fade * angle_fade;
    
    final_alpha = gridAlpha;
    final_color = gridColor;
    
    // Axis lines (X=Red, Z=Blue)
    float axis_width = line_size * 2.0;
    
    // X-axis (red)
    float x_axis = get_axis_line(grid_pos.y, derivative.y, axis_width);
    if (x_axis > 0.0) {
        final_alpha = max(final_alpha, x_axis * distance_fade);
        final_color = mix(final_color, grid_axis_x_color.rgb, x_axis);
    }
    
    // Z-axis (blue)  
    float z_axis = get_axis_line(grid_pos.x, derivative.x, axis_width);
    if (z_axis > 0.0) {
        final_alpha = max(final_alpha, z_axis * distance_fade);
        final_color = mix(final_color, grid_axis_z_color.rgb, z_axis);
    }
    
    // Output final color with alpha blending
    color = vec4(final_color, final_alpha);
}