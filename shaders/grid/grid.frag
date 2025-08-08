#version 410 core

// Fragment shader for infinite 3D grid (inspired by Blender's approach)
// Procedurally generates grid lines with anti-aliasing and distance-based fading

in vec3 world_pos;
in vec3 view_pos;
in vec2 screen_uv;

uniform vec3 camera_position;
uniform float camera_distance;
uniform vec4 grid_color;
uniform vec4 grid_emphasis_color;
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
    // Calculate derivatives for anti-aliasing (like Blender)
    vec2 grid_pos = world_pos.xz;
    vec2 derivative = abs(dFdx(grid_pos)) + abs(dFdy(grid_pos));
    
    // Distance-based fading using your archived implementation logic
    float distance_to_camera = length(view_pos);
    
    // Angle-based fading (reduce grid intensity when viewed edge-on) - your implementation
    vec3 view_dir = normalize(view_pos);
    float angle = 1.0 - abs(view_dir.y); // Fade when looking parallel to grid
    angle = pow(angle, 1.2); // Much less aggressive power curve
    float angle_fade = 1.0 - angle * 0.3; // Much reduced fade strength from 0.7 to 0.3
    
    // More gradual distance fade - starts much later and extends much further
    float distance_fade = 1.0 - smoothstep(camera_distance * 3.0, camera_distance * 8.0, distance_to_camera);
    
    vec3 final_color = vec3(0.0);
    float final_alpha = 0.0;
    
    // Multiple grid scales for detail levels using your original dynamic approach
    float scaleA = 1.0;   // Fine grid
    float scaleB = 10.0;  // Medium grid  
    float scaleC = 100.0; // Coarse grid
    
    // Calculate grid resolution for level-of-detail blending (your original approach)
    float grid_res = length(derivative);
    
    // Multi-level grid system with level-of-detail fading (your original implementation)
    float blendA = 1.0 - smoothstep(scaleA * 0.5, scaleA * 4.0, grid_res);
    float blendB = 1.0 - smoothstep(scaleB * 0.5, scaleB * 4.0, grid_res);
    float blendC = 1.0 - smoothstep(scaleC * 0.5, scaleC * 4.0, grid_res);
    
    // Apply cubic blending for smooth transitions (your original approach)
    blendA = blendA * blendA * blendA;
    blendB = blendB * blendB * blendB;
    blendC = blendC * blendC * blendC;
    
    float gridA = get_grid_line(grid_pos, derivative, scaleA);
    float gridB = get_grid_line(grid_pos, derivative, scaleB);
    float gridC = get_grid_line(grid_pos, derivative, scaleC);
    
    // Dynamic alpha values based on zoom level (your original implementation)
    // 1-unit grid: becomes more visible as you zoom in close
    float alpha1 = gridA * blendA * (0.1 + blendA * 0.4); // Dynamic from 0.1 to 0.5
    
    // 10-unit grid: always slightly more visible than 1-unit
    float alpha10 = 0.0;
    if (blendB > 0.01) {
        alpha10 = gridB * blendB * (0.3 + blendB * 0.3); // Dynamic from 0.3 to 0.6
    }
    
    // 100-unit grid: most visible at far distances
    float alpha100 = 0.0;
    if (blendC > 0.01) {
        alpha100 = gridC * blendC * (0.4 + blendC * 0.4); // Dynamic from 0.4 to 0.8
    }
    
    // Apply distance and angle fading to each grid level
    alpha1 *= distance_fade * angle_fade;
    alpha10 *= distance_fade * angle_fade;
    alpha100 *= distance_fade * angle_fade;
    
    // Combine alphas - higher level grids take precedence when visible (your original approach)
    final_alpha = max(alpha1, max(alpha10, alpha100));
    
    // Color mixing based on grid emphasis (your original approach)
    vec3 final_color_rgb = grid_color.rgb;
    if (blendB > 0.01) {
        final_color_rgb = mix(final_color_rgb, grid_emphasis_color.rgb, blendB * 0.5);
    }
    if (blendC > 0.01) {
        final_color_rgb = mix(final_color_rgb, grid_emphasis_color.rgb, blendC * 0.7);
    }
    
    final_color = final_color_rgb;
    
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
    
    color = vec4(final_color, final_alpha);
}