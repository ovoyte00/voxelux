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
    
    // Navigation widget parameters - scaled up more to better fill container
    const float axis_distance = 10.0f; // Increased further for better space usage
    const float sphere_radius = 0.72f; // Scaled proportionally (0.18f * 4.0)
    const float line_width = 3.3f;     // Fine-tuned line width for better visibility
    
    // Get axis colors from theme system
    const auto& x_color = Colors::XAxis();
    const auto& y_color = Colors::YAxis();
    const auto& z_color = Colors::ZAxis();
    
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
    
    // Store line data for screen-space rendering (like Blender's approach)
    line_data_.clear();
    float line_end_offset = sphere_radius * 0.05f; // Smaller gap so lines reach closer to spheres
    
    // Store 3D line endpoints - ensure all axes have exactly the same length
    float line_length = axis_distance - line_end_offset; // Calculate once to ensure consistency
    
    LineData x_line = {
        QVector3D(0.0f, 0.0f, 0.0f), 
        QVector3D(line_length, 0.0f, 0.0f),
        QVector3D(x_color[0], x_color[1], x_color[2]),
        Theme::instance().alpha().line
    };
    line_data_.push_back(x_line);
    
    LineData y_line = {
        QVector3D(0.0f, 0.0f, 0.0f),
        QVector3D(0.0f, line_length, 0.0f), // Normal length - aspect ratio correction handles stretching
        QVector3D(y_color[0], y_color[1], y_color[2]),
        Theme::instance().alpha().line
    };
    line_data_.push_back(y_line);
    
    LineData z_line = {
        QVector3D(0.0f, 0.0f, 0.0f),
        QVector3D(0.0f, 0.0f, line_length),
        QVector3D(z_color[0], z_color[1], z_color[2]),
        Theme::instance().alpha().line
    };
    line_data_.push_back(z_line);
    
    // Store sphere data for screen-space rendering (back-to-front for proper transparency)
    billboard_data_.clear();
    for (int idx = 0; idx < 6; idx++) {
        int i = sorted_indices[idx];
        
        // Calculate depth-based scale like Blender: Size change from back to front: 0.92f - 1.08f
        float depth_scale = ((sphere_depths[i] + 1.0f) * 0.08f) + 0.92f;
        float scaled_radius = sphere_radius * depth_scale;
        
        BillboardData billboard;
        billboard.position = QVector3D(sphere_positions[i][0], sphere_positions[i][1], sphere_positions[i][2]);
        billboard.radius = scaled_radius;
        billboard.color = QVector3D(sphere_colors[i][0], sphere_colors[i][1], sphere_colors[i][2]);
        billboard.positive = sphere_positive[i];
        billboard.depth = sphere_depths[i];
        billboard_data_.push_back(billboard);
    }
    
    // No 3D geometry needed - everything rendered in screen space now
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
    
    // Widget positioning in bottom-right (matching working implementation)
    const float widget_size = 80.0f;
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
            widget_model(i, j) = camera_view(i, j); // Copy rotation part (no transpose)
        }
    }
    widget_model(3, 3) = 1.0f;
    
    // Update shader uniforms
    widget_shader_program_.setUniformValue("model", widget_model);
    widget_shader_program_.setUniformValue("view", widget_view);
    widget_shader_program_.setUniformValue("projection", widget_projection);
    
    // Regenerate sorted widget with current camera view
    regenerateSortedWidget(camera_view);
    
    // Configure proper vertex attributes for widget rendering
    widget_vao_.bind();
    widget_vbo_.bind();
    widget_vbo_.allocate(widget_vertices_.data(), widget_vertices_.size() * sizeof(float));
    
    // Set vertex attributes (position + color)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    
    // Draw the widget
    glDrawArrays(GL_TRIANGLES, 0, widget_vertices_.size() / 7);
    
    widget_vao_.release();
    
    // ALSO render using the new screen-space method
    renderScreenSpaceElements(widget_model, widget_view, widget_projection);
    
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
    // Generate solid 3D sphere (back to working implementation for now)
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
    // Generate dual-layer billboard for negative axes (like Blender)
    // Layer 1: Transparent inner fill
    float transparent_alpha = 0.1f; // Very transparent inner
    float half_size = radius;
    
    // Inner transparent circle
    widget_vertices_.insert(widget_vertices_.end(), {
        // Triangle 1 (bottom-left, top-left, top-right)
        cx - half_size, cy - half_size, cz, color[0], color[1], color[2], transparent_alpha,
        cx - half_size, cy + half_size, cz, color[0], color[1], color[2], transparent_alpha,
        cx + half_size, cy + half_size, cz, color[0], color[1], color[2], transparent_alpha,
        
        // Triangle 2 (bottom-left, top-right, bottom-right)
        cx - half_size, cy - half_size, cz, color[0], color[1], color[2], transparent_alpha,
        cx + half_size, cy + half_size, cz, color[0], color[1], color[2], transparent_alpha,
        cx + half_size, cy - half_size, cz, color[0], color[1], color[2], transparent_alpha
    });
    
    // Layer 2: Solid outer ring (using ring geometry for now)
    float outer_radius = radius;
    float inner_radius = radius * 0.8f; // Ring thickness
    int segments = 16;
    
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        float c1 = cos(angle1), s1 = sin(angle1);
        float c2 = cos(angle2), s2 = sin(angle2);
        
        // Outer ring quad (2 triangles)
        widget_vertices_.insert(widget_vertices_.end(), {
            // Triangle 1
            cx + inner_radius * c1, cy + inner_radius * s1, cz, color[0], color[1], color[2], alpha,
            cx + outer_radius * c1, cy + outer_radius * s1, cz, color[0], color[1], color[2], alpha,
            cx + outer_radius * c2, cy + outer_radius * s2, cz, color[0], color[1], color[2], alpha,
            
            // Triangle 2
            cx + inner_radius * c1, cy + inner_radius * s1, cz, color[0], color[1], color[2], alpha,
            cx + outer_radius * c2, cy + outer_radius * s2, cz, color[0], color[1], color[2], alpha,
            cx + inner_radius * c2, cy + inner_radius * s2, cz, color[0], color[1], color[2], alpha
        });
    }
}

void ViewportNavWidget::generateSimpleLine(float x1, float y1, float z1, float x2, float y2, float z2,
                                          const float color[3], float alpha, float line_width)
{
    // Generate a simple thick line using triangles (simplified from cylindrical approach)
    // For now, create a thin rectangular tube - later can be improved with proper line rendering
    QVector3D start(x1, y1, z1);
    QVector3D end(x2, y2, z2);
    QVector3D dir = (end - start).normalized();
    
    // Find a perpendicular vector for thickness
    QVector3D perp = QVector3D::crossProduct(dir, QVector3D(0, 1, 0)).normalized();
    if (perp.length() < 0.01f) {
        perp = QVector3D::crossProduct(dir, QVector3D(1, 0, 0)).normalized();
    }
    
    // Convert line_width from pixels to world units (approximate)
    float thickness = line_width * 0.005f; // Scale factor to convert pixels to world units
    
    // Create a simple quad for the line
    QVector3D offset = perp * thickness;
    
    // Line as two triangles
    widget_vertices_.insert(widget_vertices_.end(), {
        // Triangle 1
        start.x() - offset.x(), start.y() - offset.y(), start.z() - offset.z(), color[0], color[1], color[2], alpha,
        start.x() + offset.x(), start.y() + offset.y(), start.z() + offset.z(), color[0], color[1], color[2], alpha,
        end.x() - offset.x(), end.y() - offset.y(), end.z() - offset.z(), color[0], color[1], color[2], alpha,
        
        // Triangle 2
        start.x() + offset.x(), start.y() + offset.y(), start.z() + offset.z(), color[0], color[1], color[2], alpha,
        end.x() + offset.x(), end.y() + offset.y(), end.z() + offset.z(), color[0], color[1], color[2], alpha,
        end.x() - offset.x(), end.y() - offset.y(), end.z() - offset.z(), color[0], color[1], color[2], alpha
    });
}

void ViewportNavWidget::renderScreenSpaceBillboards(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection)
{
    if (billboard_data_.empty()) return;
    
    // ===== SWITCH TO SCREEN-SPACE RENDERING =====
    
    // Set up screen-space orthographic projection
    QMatrix4x4 screen_projection;
    screen_projection.ortho(0.0f, float(parent_widget_->width()), 0.0f, float(parent_widget_->height()), -1.0f, 1.0f);
    
    // Update shader uniforms for screen space
    widget_shader_program_.setUniformValue("model", QMatrix4x4());      // Identity
    widget_shader_program_.setUniformValue("view", QMatrix4x4());       // Identity  
    widget_shader_program_.setUniformValue("projection", screen_projection);
    
    // Calculate widget positioning (matching the 3D rendering setup)
    const float widget_size = 80.0f;
    const float margin = 35.0f;
    float widget_center_x = parent_widget_->width() - margin - widget_size/2;
    float widget_center_y = margin + widget_size/2;
    
    // Prepare all billboard geometry in one batch
    std::vector<float> all_billboard_vertices;
    
    for (const auto& billboard : billboard_data_) {
        // Transform 3D position to screen space
        QMatrix4x4 mvp = projection * view * model;
        QVector4D world_pos(billboard.position, 1.0f);
        QVector4D clip_pos = mvp * world_pos;
        
        // Skip if behind camera
        if (clip_pos.w() <= 0.0f) continue;
        
        // Convert to normalized device coordinates
        QVector3D ndc = clip_pos.toVector3DAffine() / clip_pos.w();
        
        // Convert NDC to widget-local screen coordinates
        float local_x = ndc.x() * (widget_size/2.0f);
        float local_y = ndc.y() * (widget_size/2.0f);
        
        // Final screen position
        float screen_x = widget_center_x + local_x;
        float screen_y = widget_center_y + local_y;
        
        // Calculate screen-space radius with depth scaling
        float screen_radius = billboard.radius * (widget_size/2.0f) * 0.8f;
        
        // Generate geometry for this billboard
        if (billboard.positive) {
            // Solid circle for positive axes
            generateScreenSpaceCircle(screen_x, screen_y, screen_radius, 
                                    billboard.color.x(), billboard.color.y(), billboard.color.z(), 1.0f, 
                                    all_billboard_vertices);
        } else {
            // Hollow circle for negative axes
            generateScreenSpaceHollowCircle(screen_x, screen_y, screen_radius,
                                          billboard.color.x(), billboard.color.y(), billboard.color.z(),
                                          0.15f, 1.0f, all_billboard_vertices);
        }
    }
    
    // Render all billboards in one batch if we have geometry
    if (!all_billboard_vertices.empty()) {
        // Properly bind VAO for screen-space rendering
        widget_vao_.bind();
        widget_vbo_.bind();
        
        // Upload billboard data
        widget_vbo_.allocate(all_billboard_vertices.data(), all_billboard_vertices.size() * sizeof(float));
        
        // Set up vertex attributes (same layout as 3D: pos(3) + color(4))
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
        
        // Render
        int vertex_count = all_billboard_vertices.size() / 7;
        glDrawArrays(GL_TRIANGLES, 0, vertex_count);
        
        // Clean up
        widget_vbo_.release();
        widget_vao_.release();
    }
}

void ViewportNavWidget::generateScreenSpaceCircle(float cx, float cy, float radius, float r, float g, float b, float alpha, std::vector<float>& vertices)
{
    // Generate a filled circle in screen space using triangular fan
    const int segments = 16;
    
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

void ViewportNavWidget::generateScreenSpaceHollowCircle(float cx, float cy, float radius, float r, float g, float b, 
                                                       float inner_alpha, float outer_alpha, std::vector<float>& vertices)
{
    // Generate hollow circle like Blender: transparent inner fill + solid outer ring
    
    // Step 1: Generate transparent inner circle
    const int segments = 16;
    
    // Inner filled circle with transparency
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        float x1 = cx + radius * cos(angle1);
        float y1 = cy + radius * sin(angle1);
        float x2 = cx + radius * cos(angle2);
        float y2 = cy + radius * sin(angle2);
        
        // Triangle from center to edge (transparent inner)
        vertices.insert(vertices.end(), {
            cx, cy, 0.0f, r, g, b, inner_alpha,     // Center
            x1, y1, 0.0f, r, g, b, inner_alpha,     // Point 1
            x2, y2, 0.0f, r, g, b, inner_alpha      // Point 2
        });
    }
    
    // Step 2: Generate solid outer ring
    float inner_ring_radius = radius * 0.75f; // Ring starts here
    float outer_ring_radius = radius;          // Ring ends here
    
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        float c1 = cos(angle1), s1 = sin(angle1);
        float c2 = cos(angle2), s2 = sin(angle2);
        
        float ix1 = cx + inner_ring_radius * c1, iy1 = cy + inner_ring_radius * s1;
        float ox1 = cx + outer_ring_radius * c1, oy1 = cy + outer_ring_radius * s1;
        float ix2 = cx + inner_ring_radius * c2, iy2 = cy + inner_ring_radius * s2;
        float ox2 = cx + outer_ring_radius * c2, oy2 = cy + outer_ring_radius * s2;
        
        // Ring quad (2 triangles) - solid outer ring
        vertices.insert(vertices.end(), {
            // Triangle 1
            ix1, iy1, 0.0f, r, g, b, outer_alpha,
            ox1, oy1, 0.0f, r, g, b, outer_alpha,
            ox2, oy2, 0.0f, r, g, b, outer_alpha,
            
            // Triangle 2
            ix1, iy1, 0.0f, r, g, b, outer_alpha,
            ox2, oy2, 0.0f, r, g, b, outer_alpha,
            ix2, iy2, 0.0f, r, g, b, outer_alpha
        });
    }
}

void ViewportNavWidget::renderScreenSpaceElements(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection)
{
    // ===== COMPLETE SCREEN-SPACE RENDERING LIKE BLENDER =====
    
    // Set up screen-space orthographic projection
    QMatrix4x4 screen_projection;
    screen_projection.ortho(0.0f, float(parent_widget_->width()), 0.0f, float(parent_widget_->height()), -1.0f, 1.0f);
    
    // Update shader uniforms for screen space
    widget_shader_program_.setUniformValue("model", QMatrix4x4());      // Identity
    widget_shader_program_.setUniformValue("view", QMatrix4x4());       // Identity  
    widget_shader_program_.setUniformValue("projection", screen_projection);
    
    // Calculate widget positioning - bottom-right corner from user's perspective
    const float widget_container_size = 90.0f;   // Container size for positioning
    const float margin = 20.0f;                  // 25px from edges like requested
    float widget_center_x = parent_widget_->width() - (margin + widget_container_size/2);   // 25px from right edge
    float widget_center_y = margin + widget_container_size/2;                               // 25px from bottom edge
    
    // Debug: Print positioning info
    printf("DEBUG: viewport=%dx%d, container=%.1f, margin=%.1f\n", 
           (int)parent_widget_->width(), (int)parent_widget_->height(), widget_container_size, margin);
    printf("DEBUG: widget_center=%.1f,%.1f, container_bounds=[%.1f-%.1f, %.1f-%.1f]\n",
           widget_center_x, widget_center_y,
           widget_center_x - widget_container_size/2, widget_center_x + widget_container_size/2,
           widget_center_y - widget_container_size/2, widget_center_y + widget_container_size/2);
    
    // Prepare all geometry in one batch - lines first, then circles
    std::vector<float> all_screen_vertices;
    
    // DEBUG: Add flood background to visualize widget area
    float bg_left = widget_center_x - widget_container_size/2;
    float bg_right = widget_center_x + widget_container_size/2;
    float bg_bottom = widget_center_y - widget_container_size/2;
    float bg_top = widget_center_y + widget_container_size/2;
    
    // Add semi-transparent background quad (2 triangles)
    all_screen_vertices.insert(all_screen_vertices.end(), {
        // Triangle 1 (bottom-left, top-left, bottom-right)
        bg_left,  bg_bottom, 0.0f, 1.0f, 0.0f, 0.0f, 0.3f,  // Bottom-left (red)
        bg_left,  bg_top,    0.0f, 1.0f, 0.0f, 0.0f, 0.3f,  // Top-left (red)
        bg_right, bg_bottom, 0.0f, 1.0f, 0.0f, 0.0f, 0.3f,  // Bottom-right (red)
        
        // Triangle 2 (top-left, top-right, bottom-right)
        bg_left,  bg_top,    0.0f, 1.0f, 0.0f, 0.0f, 0.3f,  // Top-left (red)
        bg_right, bg_top,    0.0f, 1.0f, 0.0f, 0.0f, 0.3f,  // Top-right (red)
        bg_right, bg_bottom, 0.0f, 1.0f, 0.0f, 0.0f, 0.3f   // Bottom-right (red)
    });
    
    // ===== RENDER LINES IN SCREEN SPACE =====
    const float line_width = 2.3f; // Fine-tuned thickness for better visibility
    
    for (const auto& line : line_data_) {
        // FIX: Use rotation-only matrix like Blender to keep gizmo centered
        // Extract only the 3x3 rotation part of the view matrix
        QMatrix4x4 rotation_only_view;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                rotation_only_view(i, j) = view(i, j);
            }
        }
        rotation_only_view(3, 3) = 1.0f;  // Complete the matrix
        
        // Transform 3D line endpoints to screen space using rotation-only view
        QMatrix4x4 mvp = projection * rotation_only_view * model;
        
        // Start point
        QVector4D start_world(line.start, 1.0f);
        QVector4D start_clip = mvp * start_world;
        if (start_clip.w() <= 0.0f) continue; // Skip if behind camera
        QVector3D start_ndc = start_clip.toVector3DAffine() / start_clip.w();
        
        // End point  
        QVector4D end_world(line.end, 1.0f);
        QVector4D end_clip = mvp * end_world;
        if (end_clip.w() <= 0.0f) continue; // Skip if behind camera
        QVector3D end_ndc = end_clip.toVector3DAffine() / end_clip.w();
        
        // Convert NDC to widget-local screen coordinates with aspect ratio correction
        float widget_radius = widget_container_size * 0.5f; // Use full container size for maximum space
        
        // Apply viewport aspect ratio correction to prevent Y-axis stretching (like Blender)
        float viewport_width = parent_widget_->width();
        float viewport_height = parent_widget_->height();
        float aspect_ratio = viewport_width / viewport_height;
        
        float widget_radius_x = widget_radius;
        float widget_radius_y = widget_radius / aspect_ratio;  // Correct for viewport aspect ratio
        
        // Center the NDC coordinates so 3D origin maps to widget center
        // Origin is at NDC (-1,-1), we need it at NDC (0,0), so offset by (+1,+1)
        float centered_start_x = start_ndc.x() + 1.0f;
        float centered_start_y = start_ndc.y() + 1.0f;
        float centered_end_x = end_ndc.x() + 1.0f;
        float centered_end_y = end_ndc.y() + 1.0f;
        
        float start_x = widget_center_x + centered_start_x * widget_radius_x;
        float start_y = widget_center_y + centered_start_y * widget_radius_y;
        float end_x = widget_center_x + centered_end_x * widget_radius_x;
        float end_y = widget_center_y + centered_end_y * widget_radius_y;
        
        // DEBUG: Print NDC values for origin (0,0,0)
        if (line.start.length() < 0.1f) { // This is the origin line
            printf("DEBUG: Origin - raw_ndc=(%.2f,%.2f) centered_ndc=(%.2f,%.2f) -> screen=(%.1f,%.1f)\n",
                   start_ndc.x(), start_ndc.y(), centered_start_x, centered_start_y, start_x, start_y);
        }
        
        // Generate 2D line geometry
        generateScreenSpaceLine(start_x, start_y, end_x, end_y, line_width,
                               line.color.x(), line.color.y(), line.color.z(), line.alpha,
                               all_screen_vertices);
    }
    
    // ===== RENDER CIRCLES IN SCREEN SPACE =====
    
    for (const auto& billboard : billboard_data_) {
        // FIX: Use rotation-only matrix like Blender to keep gizmo centered
        // Extract only the 3x3 rotation part of the view matrix
        QMatrix4x4 rotation_only_view;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                rotation_only_view(i, j) = view(i, j);
            }
        }
        rotation_only_view(3, 3) = 1.0f;  // Complete the matrix
        
        // Transform 3D position to screen space using rotation-only view
        QMatrix4x4 mvp = projection * rotation_only_view * model;
        QVector4D world_pos(billboard.position, 1.0f);
        QVector4D clip_pos = mvp * world_pos;
        
        // Skip if behind camera
        if (clip_pos.w() <= 0.0f) continue;
        
        // Convert to normalized device coordinates
        QVector3D ndc = clip_pos.toVector3DAffine() / clip_pos.w();
        
        // Convert NDC to widget-local screen coordinates with aspect ratio correction
        float widget_radius = widget_container_size * 0.5f; // Use full container size for maximum space
        
        // Apply viewport aspect ratio correction to prevent Y-axis stretching (like Blender)
        float viewport_width = parent_widget_->width();
        float viewport_height = parent_widget_->height();
        float aspect_ratio = viewport_width / viewport_height;
        
        // Center the NDC coordinates so 3D origin maps to widget center  
        // Origin is at NDC (-1,-1), we need it at NDC (0,0), so offset by (+1,+1)
        float centered_ndc_x = ndc.x() + 1.0f;
        float centered_ndc_y = ndc.y() + 1.0f;
        
        float local_x = centered_ndc_x * widget_radius;
        float local_y = centered_ndc_y * widget_radius / aspect_ratio;  // Correct for viewport aspect ratio
        
        // Final screen position
        float screen_x = widget_center_x + local_x;
        float screen_y = widget_center_y + local_y;
        
        // Calculate screen-space radius with depth scaling (back to original size)
        float screen_radius = billboard.radius * widget_radius * 0.28f;
        
        // Generate geometry for this billboard
        if (billboard.positive) {
            // Solid circle for positive axes
            generateScreenSpaceCircle(screen_x, screen_y, screen_radius, 
                                    billboard.color.x(), billboard.color.y(), billboard.color.z(), 
                                    Theme::instance().alpha().positive_sphere, all_screen_vertices);
        } else {
            // Hollow circle for negative axes
            generateScreenSpaceHollowCircle(screen_x, screen_y, screen_radius,
                                          billboard.color.x(), billboard.color.y(), billboard.color.z(),
                                          Theme::instance().alpha().negative_sphere_fill, 
                                          Theme::instance().alpha().negative_sphere_ring, all_screen_vertices);
        }
    }
    
    // Render everything in one batch if we have geometry
    if (!all_screen_vertices.empty()) {
        // Properly bind VAO for screen-space rendering
        widget_vao_.bind();
        widget_vbo_.bind();
        
        // Upload all screen-space data
        widget_vbo_.allocate(all_screen_vertices.data(), all_screen_vertices.size() * sizeof(float));
        
        // Set up vertex attributes (same layout: pos(3) + color(4))
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
        
        // Render
        int vertex_count = all_screen_vertices.size() / 7;
        glDrawArrays(GL_TRIANGLES, 0, vertex_count);
        
        // Clean up
        widget_vbo_.release();
        widget_vao_.release();
    }
}

void ViewportNavWidget::generateScreenSpaceLine(float x1, float y1, float x2, float y2, float width, 
                                               float r, float g, float b, float alpha, std::vector<float>& vertices)
{
    // Generate a 2D line as a quad (like Blender's polylines)
    
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