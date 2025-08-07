#include "viewport_nav_widget.h"
#include "theme.h"
#include "typography.h"
#include <cmath>
#include <algorithm>
#include <QDebug>

ViewportNavWidget::ViewportNavWidget(QOpenGLWidget* parent)
    : ViewportWidget(parent)
{
    // Initialize sphere data
    sphere_data_[0] = {0, true,  0.0f, QVector3D( 1, 0, 0), axis_colors_[0]}; // +X
    sphere_data_[1] = {0, false, 0.0f, QVector3D(-1, 0, 0), axis_colors_[0]}; // -X
    sphere_data_[2] = {1, true,  0.0f, QVector3D(0,  1, 0), axis_colors_[1]}; // +Y
    sphere_data_[3] = {1, false, 0.0f, QVector3D(0, -1, 0), axis_colors_[1]}; // -Y
    sphere_data_[4] = {2, true,  0.0f, QVector3D(0, 0,  1), axis_colors_[2]}; // +Z
    sphere_data_[5] = {2, false, 0.0f, QVector3D(0, 0, -1), axis_colors_[2]}; // -Z
}

ViewportNavWidget::~ViewportNavWidget()
{
    cleanup();
}

void ViewportNavWidget::initialize()
{
    initializeOpenGLFunctions();
    
    // Load typography system
    Typography::instance().loadFromFile("assets/styles/typography.json");
    
    // Initialize text renderer
    text_renderer_.initialize();
    
    // Create shader program
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec4 aColor;
        
        out vec4 FragColor;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            FragColor = aColor;
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec4 FragColor;
        out vec4 outColor;
        
        void main() {
            outColor = FragColor;
        }
    )";
    
    widget_shader_program_.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    widget_shader_program_.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    widget_shader_program_.link();
    
    // Create VAO/VBO
    widget_vao_.create();
    widget_vbo_.create();
    
    sphere_vao_.create();
    sphere_vbo_.create();
}

void ViewportNavWidget::cleanup()
{
    text_renderer_.cleanup();
    
    if (widget_vao_.isCreated()) widget_vao_.destroy();
    if (widget_vbo_.isCreated()) widget_vbo_.destroy();
    if (sphere_vao_.isCreated()) sphere_vao_.destroy();
    if (sphere_vbo_.isCreated()) sphere_vbo_.destroy();
    
    ViewportWidget::cleanup();
}

void ViewportNavWidget::regenerateSortedWidget(const QMatrix4x4& camera_view)
{
    // Clear existing vertex data
    widget_vertices_.clear();
    
    // Navigation widget parameters - scaled for 90x90 widget area
    const float axis_distance = 0.85f; // Increased proportionally (0.75 * 1.125)
    const float sphere_radius = 0.73f; // Increased proportionally (0.65 * 1.125)
    const float line_width = 3.7f;     // Increased proportionally (3.3 * 1.125)
    
    // Get axis colors from theme system
    const auto& x_color = Colors::XAxis();
    const auto& y_color = Colors::YAxis();
    const auto& z_color = Colors::ZAxis();
    
    // DEBUG: Print the actual colors being used
    static bool colors_printed = false;
    if (!colors_printed) {
        qDebug() << "X-axis color:" << x_color[0] << x_color[1] << x_color[2];
        qDebug() << "Y-axis color:" << y_color[0] << y_color[1] << y_color[2];
        qDebug() << "Z-axis color:" << z_color[0] << z_color[1] << z_color[2];
        colors_printed = true;
    }
    
    // Calculate depth for each sphere using camera matrix for sorting
    float sphere_positions[6][3] = {
        { axis_distance, 0.0f, 0.0f}, // +X
        {-axis_distance, 0.0f, 0.0f}, // -X
        {0.0f,  axis_distance, 0.0f}, // +Y
        {0.0f, -axis_distance, 0.0f}, // -Y
        {0.0f, 0.0f,  axis_distance}, // +Z
        {0.0f, 0.0f, -axis_distance}  // -Z
    };
    
    float sphere_colors[6][3] = {
        {x_color[0], x_color[1], x_color[2]}, // +X
        {x_color[0], x_color[1], x_color[2]}, // -X
        {y_color[0], y_color[1], y_color[2]}, // +Y
        {y_color[0], y_color[1], y_color[2]}, // -Y
        {z_color[0], z_color[1], z_color[2]}, // +Z
        {z_color[0], z_color[1], z_color[2]}  // -Z
    };
    
    bool sphere_positive[6] = {true, false, true, false, true, false};
    float sphere_depths[6];
    
    // Calculate depth for each sphere
    float min_depth = 1e10f, max_depth = -1e10f;
    for (int i = 0; i < 6; i++) {
        // Transform sphere position by camera view matrix to get depth (Z component)
        QVector4D pos(sphere_positions[i][0], sphere_positions[i][1], sphere_positions[i][2], 1.0f);
        QVector4D transformed = camera_view * pos;
        sphere_depths[i] = transformed.z(); // Raw depth
        min_depth = std::min(min_depth, sphere_depths[i]);
        max_depth = std::max(max_depth, sphere_depths[i]);
    }
    
    // Normalize depth values to [-1, 1] range
    float depth_range = max_depth - min_depth;
    if (depth_range > 0.0f) {
        for (int i = 0; i < 6; i++) {
            sphere_depths[i] = 2.0f * ((sphere_depths[i] - min_depth) / depth_range) - 1.0f;
        }
    } else {
        for (int i = 0; i < 6; i++) {
            sphere_depths[i] = 0.0f;
        }
    }
    
    // Create sorted indices (back-to-front rendering for proper alpha blending)
    int sorted_indices[6] = {0, 1, 2, 3, 4, 5};
    std::sort(sorted_indices, sorted_indices + 6, [&](int a, int b) {
        return sphere_depths[a] < sphere_depths[b]; // Render furthest first (smallest Z)
    });
    
    // Store line data for screen-space rendering with depth sorting
    line_data_.clear();
    float line_end_offset = sphere_radius * 0.05f;
    float line_length = axis_distance - line_end_offset;
    
    // Calculate depth for each line's endpoint
    QVector3D line_endpoints[3] = {
        QVector3D( line_length, 0.0f, 0.0f),  // +X line endpoint
        QVector3D(0.0f,  line_length, 0.0f),  // +Y line endpoint
        QVector3D(0.0f, 0.0f,  line_length)   // +Z line endpoint
    };
    
    float line_depths[3];
    for (int i = 0; i < 3; i++) {
        QVector4D view_pos = camera_view * QVector4D(line_endpoints[i], 1.0f);
        line_depths[i] = view_pos.z(); // Depth in camera space
    }
    
    // Create line data array - only positive axes have lines
    LineData line_templates[3] = {
        // +X line
        {QVector3D(0.0f, 0.0f, 0.0f), QVector3D(line_length, 0.0f, 0.0f),
         QVector3D(x_color[0], x_color[1], x_color[2]), Theme::instance().alpha().line},
        // +Y line
        {QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, line_length, 0.0f),
         QVector3D(y_color[0], y_color[1], y_color[2]), Theme::instance().alpha().line},
        // +Z line
        {QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 0.0f, line_length),
         QVector3D(z_color[0], z_color[1], z_color[2]), Theme::instance().alpha().line}
    };
    
    // Sort lines by depth (furthest first, like spheres)
    int line_sorted_indices[3] = {0, 1, 2};
    std::sort(line_sorted_indices, line_sorted_indices + 3, [&](int a, int b) {
        return line_depths[a] < line_depths[b]; // Render furthest first (smallest Z)
    });
    
    // Add lines to line_data_ in sorted order
    for (int i = 0; i < 3; i++) {
        line_data_.push_back(line_templates[line_sorted_indices[i]]);
    }
    
    // Store sphere data for screen-space rendering
    billboard_data_.clear();
    for (int idx = 0; idx < 6; idx++) {
        int i = sorted_indices[idx];
        
        // Calculate depth-based scale for visual depth perception
        float depth_scale = ((sphere_depths[i] + 1.0f) * 0.08f) + 0.92f;
        float scaled_radius = sphere_radius * depth_scale;
        
        BillboardData billboard;
        billboard.position = QVector3D(sphere_positions[i][0], sphere_positions[i][1], sphere_positions[i][2]);
        billboard.radius = scaled_radius;
        billboard.color = QVector3D(sphere_colors[i][0], sphere_colors[i][1], sphere_colors[i][2]);
        billboard.positive = sphere_positive[i];
        billboard.depth = sphere_depths[i];
        // Store original sphere index for correct text mapping
        billboard.sphere_index = i;
        billboard_data_.push_back(billboard);
    }
    
    widget_vertices_.clear();
    widget_vertex_count_ = 0;
}

void ViewportNavWidget::render(const QMatrix4x4& view, const QMatrix4x4& projection)
{
    if (!visible_) return;
    
    // Save GL state
    GLboolean depth_test_enabled;
    GLboolean blend_enabled;
    glGetBooleanv(GL_DEPTH_TEST, &depth_test_enabled);
    glGetBooleanv(GL_BLEND, &blend_enabled);
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    widget_shader_program_.bind();
    
    // Regenerate widget data for screen-space rendering
    regenerateSortedWidget(view);
    
    // Single rendering pass - everything handled in screen space
    renderScreenSpaceElements(view, projection);
    
    widget_shader_program_.release();
    
    // Restore GL state
    if (depth_test_enabled) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
    if (!blend_enabled) glDisable(GL_BLEND);
}

int ViewportNavWidget::hitTest(const QPoint& pos, const QMatrix4x4& view, const QMatrix4x4& projection)
{
    // Use the same positioning as handleMouseMove
    const float widget_size = 90.0f;
    const float margin = 20.0f; // Match handleMouseMove
    const float hover_radius = 45.0f;
    float widget_center_x = parent_widget_->width() - margin - widget_size/2;
    float widget_center_y = margin + widget_size/2;
    
    float dx = pos.x() - widget_center_x;
    float dy = (parent_widget_->height() - pos.y()) - widget_center_y;
    float distance_from_center = sqrt(dx*dx + dy*dy);
    
    // Check if within circular widget bounds
    if (distance_from_center > hover_radius) {
        return -1;
    }
    
    // Convert screen position to widget-local normalized coordinates [-1, 1]
    float widget_local_x = (2.0f * dx) / widget_size; // dx is already relative to widget center
    float widget_local_y = (2.0f * dy) / widget_size; // dy is already relative to widget center
    
    // Use the same transformation as rendering: extract rotation-only view matrix
    QMatrix4x4 rotation_only_view;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            rotation_only_view(i, j) = view(i, j);
        }
    }
    rotation_only_view(3, 3) = 1.0f;
    
    // For hit testing, we need to find which 3D sphere position, when transformed
    // to screen space, is closest to our mouse position
    // Instead of trying to unproject the mouse, we'll project each sphere and compare in 2D
    
    // Build the same MVP matrix as rendering
    QMatrix4x4 widget_ortho;
    widget_ortho.ortho(-1.0f, 1.0f, -1.0f, 1.0f, -10.0f, 10.0f);
    QMatrix4x4 mvp = widget_ortho * rotation_only_view;
    
    // Check sphere hits - match the rendering parameters
    const float axis_distance = 0.85f; // Match regenerateSortedWidget value
    const float widget_radius = 45.0f * 0.45f; // widget_size/2 * 0.45 = 45 * 0.45 = 20.25
    const float sphere_base_radius = 0.73f; // Match regenerateSortedWidget sphere_radius
    
    // Sphere positions in 3D
    QVector3D sphere_positions[6] = {
        QVector3D( axis_distance, 0.0f, 0.0f),  // +X
        QVector3D(-axis_distance, 0.0f, 0.0f),  // -X
        QVector3D(0.0f,  axis_distance, 0.0f),  // +Y
        QVector3D(0.0f, -axis_distance, 0.0f),  // -Y
        QVector3D(0.0f, 0.0f,  axis_distance),  // +Z
        QVector3D(0.0f, 0.0f, -axis_distance),  // -Z
    };
    
    float closest_dist = std::numeric_limits<float>::max();
    int closest_sphere = -1;
    
    // Project each sphere to screen space and compare with mouse position in 2D
    for (int i = 0; i < 6; i++) {
        // Transform sphere position to screen space
        QVector4D world_pos(sphere_positions[i], 1.0f);
        QVector4D clip_pos = mvp * world_pos;
        
        // Skip if behind camera
        if (clip_pos.w() <= 0.0f) continue;
        
        // Convert to normalized device coordinates
        QVector3D ndc = clip_pos.toVector3DAffine() / clip_pos.w();
        
        // Calculate hit radius - larger than visual radius so you can hover anywhere on the circle
        // Make hit radius about 2.0x larger than the visual radius
        float sphere_screen_radius = sphere_base_radius * widget_radius * 0.32f * 2.0f; // 2.0x bigger for easier hovering
        // Convert to normalized space (widget is 90px, so normalized radius = screen_radius / 45)
        float sphere_radius_normalized = sphere_screen_radius / 45.0f;
        
        // DEBUG: Print radius calculations for first sphere only
        static bool debug_radius = false;
        if (i == 0 && !debug_radius) {
            qDebug() << "Radius debug - base:" << sphere_base_radius << "widget_radius:" << widget_radius << "screen_radius:" << sphere_screen_radius << "normalized:" << sphere_radius_normalized;
            debug_radius = true;
        }
        
        // Compare in 2D screen space
        float dx = ndc.x() - widget_local_x;
        float dy = ndc.y() - widget_local_y;
        float dist_2d = sqrt(dx*dx + dy*dy);
        
        // DEBUG: Print close attempts for sphere 0 (X)
        if (i == 0 && dist_2d < 0.5f) { // Print when we're somewhat close
            qDebug() << "Close to sphere 0: dist=" << dist_2d << "radius=" << sphere_radius_normalized << "ndc=" << ndc.x() << ndc.y() << "mouse=" << widget_local_x << widget_local_y;
        }
        
        if (dist_2d < sphere_radius_normalized && dist_2d < closest_dist) {
            closest_dist = dist_2d;
            closest_sphere = i;
            // DEBUG: Print when we get a hit
            qDebug() << "HIT! Sphere" << i << "radius:" << sphere_radius_normalized << "dist:" << dist_2d;
        }
    }
    
    return closest_sphere;
}

bool ViewportNavWidget::handleMouseMove(QMouseEvent* event, const QMatrix4x4& view, const QMatrix4x4& projection)
{
    if (!visible_) return false;
    
    // Check if mouse is in circular widget area
    const float widget_size = 90.0f;
    const float margin = 20.0f;
    const float hover_radius = 45.0f; // Same radius as hover background circle
    float widget_center_x = parent_widget_->width() - margin - widget_size/2;
    float widget_center_y = margin + widget_size/2;
    
    float dx = event->pos().x() - widget_center_x;
    float dy = (parent_widget_->height() - event->pos().y()) - widget_center_y;
    float distance_from_center = sqrt(dx*dx + dy*dy);
    
    bool in_widget_circle = (distance_from_center <= hover_radius);
    
    int hit = hitTest(event->pos(), view, projection);
    int new_hover_state = in_widget_circle ? (hit >= 0 ? hit : -2) : -1; // Use -2 for circle hover, -1 for no hover
    
    // Update hover state
    if (hovered_sphere_ != new_hover_state) {
        hovered_sphere_ = new_hover_state;
        if (parent_widget_) {
            parent_widget_->update();
        }
        // DEBUG: Only show when hover state changes
        qDebug() << "Hover state changed to:" << hovered_sphere_ << "(hit:" << hit << "in_circle:" << in_widget_circle << ")";
    }
    
    return in_widget_circle;
}

bool ViewportNavWidget::handleMouseRelease(QMouseEvent* event, const QMatrix4x4& view, const QMatrix4x4& projection)
{
    if (!visible_) return false;
    
    if (dragging_) {
        dragging_ = false;
        
        int hit = hitTest(event->pos(), view, projection);
        if (hit >= 0 && hit == highlighted_part_) {
            // Call axis click callback
            if (axis_click_callback_) {
                int axis = sphere_data_[hit].axis;
                bool positive = sphere_data_[hit].positive;
                axis_click_callback_(axis, positive);
            }
        }
        return true;
    }
    return false;
}

void ViewportNavWidget::setAxisColors(const QVector3D& xColor, const QVector3D& yColor, const QVector3D& zColor)
{
    axis_colors_[0] = xColor;
    axis_colors_[1] = yColor;
    axis_colors_[2] = zColor;
    
    // Update sphere colors
    sphere_data_[0].color = sphere_data_[1].color = xColor;
    sphere_data_[2].color = sphere_data_[3].color = yColor;
    sphere_data_[4].color = sphere_data_[5].color = zColor;
}

void ViewportNavWidget::generateScreenSpaceCircle(float cx, float cy, float radius, float r, float g, float b, float alpha, std::vector<float>& vertices)
{
    // Generate a smooth filled circle in screen space using triangular fan
    const int segments = 32;
    
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        float x1 = cx + radius * cos(angle1);
        float y1 = cy + radius * sin(angle1);
        float x2 = cx + radius * cos(angle2);
        float y2 = cy + radius * sin(angle2);
        
        // Triangle from center to edge
        vertices.insert(vertices.end(), {
            cx, cy, 0.0f, r, g, b, alpha,     // Center
            x1, y1, 0.0f, r, g, b, alpha,     // Point 1
            x2, y2, 0.0f, r, g, b, alpha      // Point 2
        });
    }
}

void ViewportNavWidget::generateScreenSpaceHollowCircle(float cx, float cy, float radius, 
                                                       float bg_r, float bg_g, float bg_b, float bg_alpha,
                                                       float axis_r, float axis_g, float axis_b, float axis_alpha,
                                                       float ring_r, float ring_g, float ring_b, float ring_alpha,
                                                       std::vector<float>& vertices)
{
    // Generate 3-layer hollow circle: background + axis color + ring
    const int segments = 16;
    
    // Layer 1: Background circle
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        float x1 = cx + radius * cos(angle1);
        float y1 = cy + radius * sin(angle1);
        float x2 = cx + radius * cos(angle2);
        float y2 = cy + radius * sin(angle2);
        
        // Background circle (bottom layer)
        vertices.insert(vertices.end(), {
            cx, cy, 0.0f, bg_r, bg_g, bg_b, bg_alpha,
            x1, y1, 0.0f, bg_r, bg_g, bg_b, bg_alpha,
            x2, y2, 0.0f, bg_r, bg_g, bg_b, bg_alpha
        });
    }
    
    // Layer 2: Axis color circle
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        float x1 = cx + radius * cos(angle1);
        float y1 = cy + radius * sin(angle1);
        float x2 = cx + radius * cos(angle2);
        float y2 = cy + radius * sin(angle2);
        
        // Axis color circle (middle layer)
        vertices.insert(vertices.end(), {
            cx, cy, 0.0f, axis_r, axis_g, axis_b, axis_alpha,
            x1, y1, 0.0f, axis_r, axis_g, axis_b, axis_alpha,
            x2, y2, 0.0f, axis_r, axis_g, axis_b, axis_alpha
        });
    }
    
    // Layer 3: Outer ring
    float inner_ring_radius = radius * 0.90f;
    float outer_ring_radius = radius;
    
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        float c1 = cos(angle1), s1 = sin(angle1);
        float c2 = cos(angle2), s2 = sin(angle2);
        
        float ix1 = cx + inner_ring_radius * c1, iy1 = cy + inner_ring_radius * s1;
        float ox1 = cx + outer_ring_radius * c1, oy1 = cy + outer_ring_radius * s1;
        float ix2 = cx + inner_ring_radius * c2, iy2 = cy + inner_ring_radius * s2;
        float ox2 = cx + outer_ring_radius * c2, oy2 = cy + outer_ring_radius * s2;
        
        // Ring quad (2 triangles)
        vertices.insert(vertices.end(), {
            // Triangle 1
            ix1, iy1, 0.0f, ring_r, ring_g, ring_b, ring_alpha,
            ox1, oy1, 0.0f, ring_r, ring_g, ring_b, ring_alpha,
            ox2, oy2, 0.0f, ring_r, ring_g, ring_b, ring_alpha,
            
            // Triangle 2
            ix1, iy1, 0.0f, ring_r, ring_g, ring_b, ring_alpha,
            ox2, oy2, 0.0f, ring_r, ring_g, ring_b, ring_alpha,
            ix2, iy2, 0.0f, ring_r, ring_g, ring_b, ring_alpha
        });
    }
}

void ViewportNavWidget::generateScreenSpaceLine(float x1, float y1, float x2, float y2, float width,
                                               float r, float g, float b, float alpha, std::vector<float>& vertices)
{
    // Generate a 2D line as a quad with proper thickness
    
    // Calculate line direction and perpendicular
    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = sqrt(dx*dx + dy*dy);
    
    if (length < 0.001f) return; // Skip degenerate lines
    
    // Normalize direction
    dx /= length;
    dy /= length;
    
    // Calculate perpendicular (for line thickness)
    float px = -dy * (width * 0.5f);
    float py = dx * (width * 0.5f);
    
    // Generate quad vertices (2 triangles)
    vertices.insert(vertices.end(), {
        // Triangle 1
        x1 - px, y1 - py, 0.0f, r, g, b, alpha,  // Bottom-left
        x1 + px, y1 + py, 0.0f, r, g, b, alpha,  // Top-left
        x2 - px, y2 - py, 0.0f, r, g, b, alpha,  // Bottom-right
        
        // Triangle 2  
        x1 + px, y1 + py, 0.0f, r, g, b, alpha,  // Top-left
        x2 + px, y2 + py, 0.0f, r, g, b, alpha,  // Top-right
        x2 - px, y2 - py, 0.0f, r, g, b, alpha   // Bottom-right
    });
}

void ViewportNavWidget::renderScreenSpaceElements(const QMatrix4x4& view, const QMatrix4x4& projection)
{
    // Set up screen-space rendering with 90x90 widget area
    const float widget_size = 90.0f;
    const float margin = 20.0f;
    const float widget_center_x = float(parent_widget_->width()) - margin - widget_size * 0.5f;
    const float widget_center_y = margin + widget_size * 0.5f;
    const float widget_radius = widget_size * 0.45f; // Increased proportionally (0.4 * 1.125)
    
    QMatrix4x4 screen_projection;
    screen_projection.ortho(0.0f, float(parent_widget_->width()), 0.0f, float(parent_widget_->height()), -1.0f, 1.0f);
    
    widget_shader_program_.bind();
    widget_shader_program_.setUniformValue("model", QMatrix4x4());
    widget_shader_program_.setUniformValue("view", QMatrix4x4());
    widget_shader_program_.setUniformValue("projection", screen_projection);
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Render hover background when mouse is anywhere in the circular widget area
    if (hovered_sphere_ >= 0 || hovered_sphere_ == -2) {
        const float hover_radius = 45.0f; // 45px radius for 90px diameter, fitting 90x90 widget area
        const float hover_alpha = Theme::instance().alpha().hover_background;
        
        std::vector<float> hover_vertices;
        generateScreenSpaceCircle(widget_center_x, widget_center_y, hover_radius, 
                                1.0f, 1.0f, 1.0f, hover_alpha, hover_vertices);
        
        if (!hover_vertices.empty()) {
            widget_vao_.bind();
            widget_vbo_.bind();
            widget_vbo_.allocate(hover_vertices.data(), hover_vertices.size() * sizeof(float));
            
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
            
            glDrawArrays(GL_TRIANGLES, 0, hover_vertices.size() / 7);
            
            widget_vbo_.release();
            widget_vao_.release();
        }
    }
    
    // Render axis elements in depth order
    QMatrix4x4 rotation_only_view;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            rotation_only_view(i, j) = view(i, j);
        }
    }
    rotation_only_view(3, 3) = 1.0f;
    
    QMatrix4x4 widget_ortho;
    widget_ortho.ortho(-1.0f, 1.0f, -1.0f, 1.0f, -10.0f, 10.0f);
    QMatrix4x4 mvp = widget_ortho * rotation_only_view;
    
    // Render lines first
    for (const auto& line : line_data_) {
        QVector4D start_clip = mvp * QVector4D(line.start, 1.0f);
        QVector4D end_clip = mvp * QVector4D(line.end, 1.0f);
        
        if (start_clip.w() > 0.0f && end_clip.w() > 0.0f) {
            QVector3D start_ndc = start_clip.toVector3DAffine() / start_clip.w();
            QVector3D end_ndc = end_clip.toVector3DAffine() / end_clip.w();
            
            float start_x = widget_center_x + start_ndc.x() * widget_radius;
            float start_y = widget_center_y + start_ndc.y() * widget_radius;
            float end_x = widget_center_x + end_ndc.x() * widget_radius;
            float end_y = widget_center_y + end_ndc.y() * widget_radius;
            
            std::vector<float> line_vertices;
            generateScreenSpaceLine(start_x, start_y, end_x, end_y, 2.3f,
                                   line.color.x(), line.color.y(), line.color.z(), line.alpha,
                                   line_vertices);
            
            if (!line_vertices.empty()) {
                widget_vao_.bind();
                widget_vbo_.bind();
                widget_vbo_.allocate(line_vertices.data(), line_vertices.size() * sizeof(float));
                
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
                
                glDrawArrays(GL_TRIANGLES, 0, line_vertices.size() / 7);
                
                widget_vbo_.release();
                widget_vao_.release();
            }
        }
    }
    
    // Render spheres with text
    for (int i = 0; i < billboard_data_.size(); i++) {
        const auto& billboard = billboard_data_[i];
        
        QVector4D world_pos(billboard.position, 1.0f);
        QVector4D clip_pos = mvp * world_pos;
        
        if (clip_pos.w() > 0.0f) {
            QVector3D ndc = clip_pos.toVector3DAffine() / clip_pos.w();
            
            float screen_x = widget_center_x + ndc.x() * widget_radius;
            float screen_y = widget_center_y + ndc.y() * widget_radius;
            float screen_radius = billboard.radius * widget_radius * 0.32f; // Increased proportionally (0.28 * 1.125)
            
            std::vector<float> sphere_vertices;
            
            if (billboard.positive) {
                generateScreenSpaceCircle(screen_x, screen_y, screen_radius, 
                                        billboard.color.x(), billboard.color.y(), billboard.color.z(), 
                                        Theme::instance().alpha().positive_sphere, sphere_vertices);
            } else {
                float depth_normalized = (billboard.depth + 1.0f) * 0.5f;
                float background_alpha = depth_normalized;
                const auto& bg_color = Colors::Background();
                
                generateScreenSpaceHollowCircle(screen_x, screen_y, screen_radius,
                                              bg_color[0], bg_color[1], bg_color[2], background_alpha,
                                              billboard.color.x(), billboard.color.y(), billboard.color.z(), 0.25f,
                                              billboard.color.x(), billboard.color.y(), billboard.color.z(), 
                                              Theme::instance().alpha().negative_sphere_ring, sphere_vertices);
            }
            
            if (!sphere_vertices.empty()) {
                widget_vao_.bind();
                widget_vbo_.bind();
                widget_vbo_.allocate(sphere_vertices.data(), sphere_vertices.size() * sizeof(float));
                
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
                
                glDrawArrays(GL_TRIANGLES, 0, sphere_vertices.size() / 7);
                
                widget_vbo_.release();
                widget_vao_.release();
            }
            
            // Render text for this sphere - find correct position for the specific sphere_index
            renderTextForSphere(i, billboard.sphere_index, screen_x, screen_y, view, projection);
        }
    }
}

void ViewportNavWidget::renderTextForSphere(int billboard_index, int sphere_index, float screen_x, float screen_y, const QMatrix4x4& view, const QMatrix4x4& projection)
{
    if (!parent_widget_ || billboard_index >= billboard_data_.size()) return;
    
    // Sphere labels - match the increased axis distance
    const float axis_distance = 0.85f; // Match regenerateSortedWidget value
    struct SphereLabel {
        QVector3D position;
        QString label;
        bool positive;
    };
    
    SphereLabel spheres[6] = {
        {QVector3D( axis_distance, 0.0f, 0.0f), "X",  true},  // +X
        {QVector3D(-axis_distance, 0.0f, 0.0f), "-X", false}, // -X
        {QVector3D(0.0f,  axis_distance, 0.0f), "Y",  true},  // +Y
        {QVector3D(0.0f, -axis_distance, 0.0f), "-Y", false}, // -Y
        {QVector3D(0.0f, 0.0f,  axis_distance), "Z",  true},  // +Z
        {QVector3D(0.0f, 0.0f, -axis_distance), "-Z", false}  // -Z
    };
    
    const auto& billboard = billboard_data_[billboard_index]; // Use billboard_index for position
    
    // Find matching sphere label by position
    int label_index = -1;
    for (int i = 0; i < 6; i++) {
        float dist = (spheres[i].position - billboard.position).length();
        if (dist < 0.1f) {
            label_index = i;
            break;
        }
    }
    
    if (label_index < 0) return;
    
    // Determine visibility and color using sphere_index for hover detection
    bool is_hovered = (hovered_sphere_ == sphere_index);
    bool show_label = false;
    QVector3D text_color;
    
    // DEBUG: Print hover state (only when hovering)
    if (is_hovered) {
        qDebug() << "Sphere" << sphere_index << "is hovered - should show white text";
    }
    
    if (spheres[label_index].positive) {
        // Positive axes: always show text
        show_label = true;
        if (is_hovered) {
            text_color = QVector3D(1.0f, 1.0f, 1.0f); // White
        } else {
            text_color = QVector3D(0.0f, 0.0f, 0.0f); // Black
        }
    } else {
        // Negative axes: only show when hovered
        show_label = is_hovered;
        text_color = QVector3D(1.0f, 1.0f, 1.0f); // White when hovered
    }
    
    if (show_label) {
        QMatrix4x4 screen_projection;
        screen_projection.ortho(0.0f, float(parent_widget_->width()), 0.0f, float(parent_widget_->height()), -1.0f, 1.0f);
        
        // Use smaller scale for negative axes to fit better in the sphere
        float text_scale = spheres[label_index].positive ? 1.0f : 0.8f;
        text_renderer_.renderTextCentered(spheres[label_index].label, screen_x, screen_y, text_scale, 
                                        text_color, screen_projection);
        
        // Re-bind widget shader context
        widget_shader_program_.bind();
        widget_shader_program_.setUniformValue("model", QMatrix4x4());
        widget_shader_program_.setUniformValue("view", QMatrix4x4());
        widget_shader_program_.setUniformValue("projection", screen_projection);
    }
}

void ViewportNavWidget::renderHoverBackground(float x, float y, const QMatrix4x4& projection)
{
    std::vector<float> hover_vertices;
    const float hover_radius = 45.0f; // 45px radius for 90px diameter, fitting 90x90 widget area
    const float hover_alpha = Theme::instance().alpha().hover_background;
    
    generateScreenSpaceCircle(x, y, hover_radius, 1.0f, 1.0f, 1.0f, hover_alpha, hover_vertices);
    
    if (!hover_vertices.empty()) {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        widget_shader_program_.bind();
        widget_shader_program_.setUniformValue("model", QMatrix4x4());
        widget_shader_program_.setUniformValue("view", QMatrix4x4());
        widget_shader_program_.setUniformValue("projection", projection);
        
        widget_vao_.bind();
        widget_vbo_.bind();
        widget_vbo_.allocate(hover_vertices.data(), hover_vertices.size() * sizeof(float));
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
        
        glDrawArrays(GL_TRIANGLES, 0, hover_vertices.size() / 7);
        
        widget_vbo_.release();
        widget_vao_.release();
        widget_shader_program_.release();
    }
}