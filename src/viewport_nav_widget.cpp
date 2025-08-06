#include "viewport_nav_widget.h"
#include "theme.h"
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
    
    // Create geometry
    createWidgetGeometry();
    createSphereGeometry();
}

void ViewportNavWidget::cleanup()
{
    if (widget_vao_.isCreated()) widget_vao_.destroy();
    if (widget_vbo_.isCreated()) widget_vbo_.destroy();
    if (sphere_vao_.isCreated()) sphere_vao_.destroy();
    if (sphere_vbo_.isCreated()) sphere_vbo_.destroy();
    
    ViewportWidget::cleanup();
}

void ViewportNavWidget::createWidgetGeometry()
{
    widget_vertices_.clear();
    
    // Helper lambda to add a cylindrical line
    auto add_cylindrical_line = [this](float x1, float y1, float z1, float x2, float y2, float z2, 
                                       float r, float g, float b, float a) {
        float radius = axis_thickness_ / 2.0f;
        int segments = 8;
        
        float dx = x2 - x1, dy = y2 - y1, dz = z2 - z1;
        float length = sqrt(dx*dx + dy*dy + dz*dz);
        if (length < 0.001f) return;
        
        dx /= length; dy /= length; dz /= length;
        
        // Find perpendicular vectors
        float ux = (std::abs(dx) < 0.9f) ? 1.0f : 0.0f;
        float uy = (std::abs(dy) < 0.9f) ? 1.0f : 0.0f; 
        float uz = (std::abs(dz) < 0.9f) ? 1.0f : 0.0f;
        
        float px = dy * uz - dz * uy;
        float py = dz * ux - dx * uz;
        float pz = dx * uy - dy * ux;
        float plen = sqrt(px*px + py*py + pz*pz);
        px /= plen; py /= plen; pz /= plen;
        
        float qx = dy * pz - dz * py;
        float qy = dz * px - dx * pz;
        float qz = dx * py - dy * px;
        
        // Generate cylinder triangles
        for (int i = 0; i < segments; i++) {
            float angle1 = 2.0f * M_PI * i / segments;
            float angle2 = 2.0f * M_PI * (i + 1) / segments;
            
            float c1 = cos(angle1) * radius, s1 = sin(angle1) * radius;
            float c2 = cos(angle2) * radius, s2 = sin(angle2) * radius;
            
            float v1x = x1 + px * c1 + qx * s1, v1y = y1 + py * c1 + qy * s1, v1z = z1 + pz * c1 + qz * s1;
            float v2x = x2 + px * c1 + qx * s1, v2y = y2 + py * c1 + qy * s1, v2z = z2 + pz * c1 + qz * s1;
            float v3x = x2 + px * c2 + qx * s2, v3y = y2 + py * c2 + qy * s2, v3z = z2 + pz * c2 + qz * s2;
            float v4x = x1 + px * c2 + qx * s2, v4y = y1 + py * c2 + qy * s2, v4z = z1 + pz * c2 + qz * s2;
            
            // Triangle 1
            widget_vertices_.insert(widget_vertices_.end(), {
                v1x, v1y, v1z, r, g, b, a,
                v2x, v2y, v2z, r, g, b, a,
                v3x, v3y, v3z, r, g, b, a,
            });
            
            // Triangle 2
            widget_vertices_.insert(widget_vertices_.end(), {
                v1x, v1y, v1z, r, g, b, a,
                v3x, v3y, v3z, r, g, b, a,
                v4x, v4y, v4z, r, g, b, a,
            });
        }
    };
    
    // Draw axis lines
    float fade_start = axis_length_ * 0.6f;
    float fade_length = axis_length_ - fade_start;
    
    // X axis (red)
    add_cylindrical_line(0, 0, 0,  axis_length_, 0, 0, 
                        axis_colors_[0].x(), axis_colors_[0].y(), axis_colors_[0].z(), 0.8f);
    add_cylindrical_line(0, 0, 0, -axis_length_, 0, 0, 
                        axis_colors_[0].x() * 0.5f, axis_colors_[0].y() * 0.5f, axis_colors_[0].z() * 0.5f, 0.5f);
    
    // Y axis (green)
    add_cylindrical_line(0, 0, 0, 0,  axis_length_, 0, 
                        axis_colors_[1].x(), axis_colors_[1].y(), axis_colors_[1].z(), 0.8f);
    add_cylindrical_line(0, 0, 0, 0, -axis_length_, 0, 
                        axis_colors_[1].x() * 0.5f, axis_colors_[1].y() * 0.5f, axis_colors_[1].z() * 0.5f, 0.5f);
    
    // Z axis (blue)
    add_cylindrical_line(0, 0, 0, 0, 0,  axis_length_, 
                        axis_colors_[2].x(), axis_colors_[2].y(), axis_colors_[2].z(), 0.8f);
    add_cylindrical_line(0, 0, 0, 0, 0, -axis_length_, 
                        axis_colors_[2].x() * 0.5f, axis_colors_[2].y() * 0.5f, axis_colors_[2].z() * 0.5f, 0.5f);
    
    widget_vertex_count_ = widget_vertices_.size() / 7;
    
    // Upload to GPU
    widget_vao_.bind();
    widget_vbo_.bind();
    widget_vbo_.allocate(widget_vertices_.data(), widget_vertices_.size() * sizeof(float));
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    
    // Color attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    
    widget_vbo_.release();
    widget_vao_.release();
}

void ViewportNavWidget::createSphereGeometry()
{
    sphere_template_.clear();
    
    // Generate sphere vertices
    for (int lat = 0; lat <= sphere_segments_; lat++) {
        float theta = lat * M_PI / sphere_segments_;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        
        for (int lon = 0; lon <= sphere_segments_; lon++) {
            float phi = lon * 2 * M_PI / sphere_segments_;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;
            
            sphere_template_.push_back(x * sphere_radius_);
            sphere_template_.push_back(y * sphere_radius_);
            sphere_template_.push_back(z * sphere_radius_);
        }
    }
}

void ViewportNavWidget::regenerateSortedWidget(const QMatrix4x4& camera_view)
{
    // Clear existing vertex data
    widget_vertices_.clear();
    
    // Navigation widget parameters matching the working implementation
    const float axis_distance = 0.85f; // Distance from center to spheres
    const float sphere_radius = 0.22f; // Sphere size (increased)
    const float line_radius = 0.025f;  // Slightly thicker cylindrical lines
    const float line_end_offset = sphere_radius * 0.1f; // Small offset so line touches sphere
    
    // Get axis colors from theme system
    const auto& x_color = Colors::XAxis();
    const auto& y_color = Colors::YAxis();
    const auto& z_color = Colors::ZAxis();
    
    // Generate cylindrical lines from center to positive axes only (like the working implementation)
    // +X axis line (center to +X - pointing toward red sphere)
    generateCylindricalLine(0.0f, 0.0f, 0.0f, 
                           axis_distance - line_end_offset, 0.0f, 0.0f,
                           x_color.data(), Theme::instance().alpha().line, line_radius);
    // +Y axis line (center to +Y)
    generateCylindricalLine(0.0f, 0.0f, 0.0f,
                           0.0f, axis_distance - line_end_offset, 0.0f,
                           y_color.data(), Theme::instance().alpha().line, line_radius);
    // +Z axis line (center to +Z)
    generateCylindricalLine(0.0f, 0.0f, 0.0f,
                           0.0f, 0.0f, axis_distance - line_end_offset,
                           z_color.data(), Theme::instance().alpha().line, line_radius);
    
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
    
    float min_depth = 1e10f, max_depth = -1e10f;
    for (int i = 0; i < 6; i++) {
        // Transform sphere position by camera view matrix to get depth (Z component)
        QVector4D pos(sphere_positions[i][0], sphere_positions[i][1], sphere_positions[i][2], 1.0f);
        QVector4D transformed = camera_view * pos;
        sphere_depths[i] = transformed.z(); // Raw depth
        min_depth = std::min(min_depth, sphere_depths[i]);
        max_depth = std::max(max_depth, sphere_depths[i]);
    }
    
    // Normalize depth values to [-1, 1] range like the working implementation
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
    
    // Generate spheres in sorted order (back-to-front for proper transparency)
    for (int idx = 0; idx < 6; idx++) {
        int i = sorted_indices[idx];
        
        if (sphere_positive[i]) {
            // Solid sphere for positive axes
            generateSphere(sphere_positions[i][0], sphere_positions[i][1], sphere_positions[i][2],
                          sphere_radius, sphere_colors[i], Colors::PositiveSphere(), 16, 16);
        } else {
            // Hollow sphere for negative axes with lower fill opacity and visible outline
            generateSphereOutline(sphere_positions[i][0], sphere_positions[i][1], sphere_positions[i][2],
                                 sphere_radius, sphere_colors[i], Colors::NegativeFill(), 12, 12, 0.03f);
        }
    }
    
    // Update vertex count
    widget_vertex_count_ = widget_vertices_.size() / 7;
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
    
    // Widget positioning in bottom-right (matching working implementation)
    const float widget_size = 80.0f; // Larger size like Blender
    const float margin = 35.0f; // Margin from edges
    float widget_center_x = parent_widget_->width() - margin - widget_size/2;
    float widget_center_y = margin + widget_size/2;
    
    // Set up matrices like the working implementation
    QMatrix4x4 widget_model, widget_view, widget_projection;
    
    // Screen-space orthographic projection
    widget_projection.ortho(0.0f, float(parent_widget_->width()), 0.0f, float(parent_widget_->height()), -100.0f, 100.0f);
    
    // Position the widget
    widget_view.translate(widget_center_x, widget_center_y, 0.0f);
    widget_view.scale(widget_size/2.0f);
    
    // Apply camera rotation - extract just the rotation part for the widget
    QMatrix4x4 camera_view = view;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            widget_model(i, j) = camera_view(i, j);
        }
    }
    widget_model(3, 3) = 1.0f;
    
    widget_shader_program_.setUniformValue("model", widget_model);
    widget_shader_program_.setUniformValue("view", widget_view);
    widget_shader_program_.setUniformValue("projection", widget_projection);
    
    // Regenerate sorted widget geometry (like the working implementation)
    regenerateSortedWidget(camera_view);
    
    // Render main widget
    widget_vao_.bind();
    if (widget_vertex_count_ > 0) {
        // Update buffer with sorted geometry
        widget_vbo_.bind();
        widget_vbo_.allocate(widget_vertices_.data(), widget_vertices_.size() * sizeof(float));
        widget_vbo_.release();
        
        glDrawArrays(GL_TRIANGLES, 0, widget_vertex_count_);
    }
    widget_vao_.release();
    
    widget_shader_program_.release();
    
    // Restore GL state
    if (depth_test_enabled) glEnable(GL_DEPTH_TEST);
    if (!blend_enabled) glDisable(GL_BLEND);
}

int ViewportNavWidget::hitTest(const QPoint& pos, const QMatrix4x4& view, const QMatrix4x4& projection)
{
    // Use the same positioning as the render method
    const float widget_size = 80.0f;
    const float margin = 30.0f;
    float widget_center_x = parent_widget_->width() - margin - widget_size/2;
    float widget_center_y = margin + widget_size/2;
    
    float dx = pos.x() - widget_center_x;
    float dy = (parent_widget_->height() - pos.y()) - widget_center_y;
    
    // Check if within widget bounds
    if (std::abs(dx) > widget_size/2 || std::abs(dy) > widget_size/2) {
        return -1;
    }
    
    // Normalize to widget space [-1, 1]
    dx = dx / (widget_size/2);
    dy = dy / (widget_size/2);
    
    // Apply inverse camera rotation
    QMatrix4x4 camera_view = view;
    QMatrix4x4 inv_rot;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            inv_rot(i, j) = camera_view(j, i); // Transpose for rotation inverse
        }
    }
    inv_rot(3, 3) = 1.0f;
    
    QVector3D mouse_pos(dx, dy, 0);
    mouse_pos = inv_rot.mapVector(mouse_pos);
    
    // Check sphere hits - sphere positions match the render method
    const float axis_distance = 1.0f;
    const float sphere_size = 0.12f;
    const float hit_radius = sphere_size * 1.5f;
    
    // Sphere positions (matching the add_circle_sphere calls in render)
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
    
    for (int i = 0; i < 6; i++) {
        float dist = (sphere_positions[i] - mouse_pos).length();
        
        if (dist < hit_radius && dist < closest_dist) {
            closest_dist = dist;
            closest_sphere = i;
        }
    }
    
    return closest_sphere;
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
    
    // Regenerate geometry with new colors
    createWidgetGeometry();
}// Helper methods for ViewportNavWidget - to be appended to the main file

void ViewportNavWidget::generateCylindricalLine(float x1, float y1, float z1, float x2, float y2, float z2,
                                              const float color[3], float alpha, float radius)
{
    // Create a proper 3D cylinder for the line (matching working implementation)
    QVector3D start(x1, y1, z1);
    QVector3D end(x2, y2, z2);
    QVector3D dir = (end - start).normalized();
    
    // Find perpendicular vectors for cylinder cross-section
    QVector3D perp1 = QVector3D::crossProduct(dir, QVector3D(0, 1, 0)).normalized();
    if (perp1.length() < 0.01f) {
        perp1 = QVector3D::crossProduct(dir, QVector3D(1, 0, 0)).normalized();
    }
    QVector3D perp2 = QVector3D::crossProduct(dir, perp1).normalized();
    
    // Generate cylinder with 8 sides
    const int sides = 8;
    for (int i = 0; i < sides; i++) {
        float angle1 = 2.0f * M_PI * i / sides;
        float angle2 = 2.0f * M_PI * (i + 1) / sides;
        
        // Calculate positions on cylinder
        QVector3D p1 = perp1 * (radius * cos(angle1)) + perp2 * (radius * sin(angle1));
        QVector3D p2 = perp1 * (radius * cos(angle2)) + perp2 * (radius * sin(angle2));
        
        // Two triangles for this side of cylinder
        widget_vertices_.insert(widget_vertices_.end(), {
            // First triangle
            start.x() + p1.x(), start.y() + p1.y(), start.z() + p1.z(), color[0], color[1], color[2], alpha,
            end.x() + p1.x(), end.y() + p1.y(), end.z() + p1.z(), color[0], color[1], color[2], alpha,
            start.x() + p2.x(), start.y() + p2.y(), start.z() + p2.z(), color[0], color[1], color[2], alpha,
            
            // Second triangle
            start.x() + p2.x(), start.y() + p2.y(), start.z() + p2.z(), color[0], color[1], color[2], alpha,
            end.x() + p1.x(), end.y() + p1.y(), end.z() + p1.z(), color[0], color[1], color[2], alpha,
            end.x() + p2.x(), end.y() + p2.y(), end.z() + p2.z(), color[0], color[1], color[2], alpha
        });
    }
}

void ViewportNavWidget::generateSphere(float cx, float cy, float cz, float radius,
                                      const float color[3], float alpha, int lat_segments, int lon_segments)
{
    // Generate solid 3D sphere (matching working implementation)
    for (int lat = 0; lat < lat_segments; lat++) {
        for (int lon = 0; lon < lon_segments; lon++) {
            // Calculate sphere coordinates
            float phi1 = M_PI * lat / lat_segments;
            float phi2 = M_PI * (lat + 1) / lat_segments;
            float theta1 = 2.0f * M_PI * lon / lon_segments;
            float theta2 = 2.0f * M_PI * (lon + 1) / lon_segments;
            
            // Calculate vertices
            QVector3D v1(cx + radius * sin(phi1) * cos(theta1),
                        cy + radius * cos(phi1),
                        cz + radius * sin(phi1) * sin(theta1));
            QVector3D v2(cx + radius * sin(phi2) * cos(theta1),
                        cy + radius * cos(phi2),
                        cz + radius * sin(phi2) * sin(theta1));
            QVector3D v3(cx + radius * sin(phi2) * cos(theta2),
                        cy + radius * cos(phi2),
                        cz + radius * sin(phi2) * sin(theta2));
            QVector3D v4(cx + radius * sin(phi1) * cos(theta2),
                        cy + radius * cos(phi1),
                        cz + radius * sin(phi1) * sin(theta2));
            
            // Add triangles
            widget_vertices_.insert(widget_vertices_.end(), {
                // Triangle 1
                v1.x(), v1.y(), v1.z(), color[0], color[1], color[2], alpha,
                v2.x(), v2.y(), v2.z(), color[0], color[1], color[2], alpha,
                v3.x(), v3.y(), v3.z(), color[0], color[1], color[2], alpha,
                
                // Triangle 2
                v1.x(), v1.y(), v1.z(), color[0], color[1], color[2], alpha,
                v3.x(), v3.y(), v3.z(), color[0], color[1], color[2], alpha,
                v4.x(), v4.y(), v4.z(), color[0], color[1], color[2], alpha
            });
        }
    }
}

void ViewportNavWidget::generateSphereOutline(float cx, float cy, float cz, float radius,
                                             const float color[3], float alpha, int lat_segments, int lon_segments, float outline_width)
{
    // Generate hollow sphere outline (ring-like appearance) for negative axes
    const float inner_radius = radius * 0.6f;
    
    for (int lat = 0; lat < lat_segments; lat++) {
        for (int lon = 0; lon < lon_segments; lon++) {
            float phi1 = M_PI * lat / lat_segments;
            float phi2 = M_PI * (lat + 1) / lat_segments;
            float theta1 = 2.0f * M_PI * lon / lon_segments;
            float theta2 = 2.0f * M_PI * (lon + 1) / lon_segments;
            
            // Outer ring vertices
            QVector3D v1_outer(cx + radius * sin(phi1) * cos(theta1),
                              cy + radius * cos(phi1),
                              cz + radius * sin(phi1) * sin(theta1));
            QVector3D v2_outer(cx + radius * sin(phi2) * cos(theta1),
                              cy + radius * cos(phi2),
                              cz + radius * sin(phi2) * sin(theta1));
            
            // Inner ring vertices
            QVector3D v1_inner(cx + inner_radius * sin(phi1) * cos(theta1),
                              cy + inner_radius * cos(phi1),
                              cz + inner_radius * sin(phi1) * sin(theta1));
            QVector3D v2_inner(cx + inner_radius * sin(phi2) * cos(theta1),
                              cy + inner_radius * cos(phi2),
                              cz + inner_radius * sin(phi2) * sin(theta1));
            
            // Add ring triangles (simplified for ring appearance)
            widget_vertices_.insert(widget_vertices_.end(), {
                v1_outer.x(), v1_outer.y(), v1_outer.z(), color[0], color[1], color[2], alpha,
                v2_outer.x(), v2_outer.y(), v2_outer.z(), color[0], color[1], color[2], alpha,
                v1_inner.x(), v1_inner.y(), v1_inner.z(), color[0], color[1], color[2], alpha,
                
                v2_outer.x(), v2_outer.y(), v2_outer.z(), color[0], color[1], color[2], alpha,
                v2_inner.x(), v2_inner.y(), v2_inner.z(), color[0], color[1], color[2], alpha,
                v1_inner.x(), v1_inner.y(), v1_inner.z(), color[0], color[1], color[2], alpha
            });
        }
    }
}