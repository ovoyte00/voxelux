#include <QApplication>
#include <QMainWindow>
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <algorithm>
#include <cfloat>
#include "theme.h"
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QMessageBox>
#include <QTimer>
#include <QtMath>
#include <QDebug>
#include <QGestureEvent>
#include <QPinchGesture>
#include <QPanGesture>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QSurfaceFormat>
#ifdef Q_OS_MACOS
#include <QSettings>
#endif
#include <array>
#include <vector>
#include <memory>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Professional 3D camera system with turntable navigation
class VoxelCamera {
private:
    QQuaternion rotation_;   // Quaternion-based rotation for smooth 360° movement
    float distance_ = 500.0f; // Further back to see more building context (100x100 voxel area)
    float target_x_ = 0.0f, target_y_ = 0.0f, target_z_ = 0.0f;
    
public:
    VoxelCamera() {
        // Initialize with professional 3D editing view: looking down at grid from above
        // Start with view showing +X (right) and +Z (forward) - standard 3D orientation
        rotation_ = QQuaternion::fromEulerAngles(-30.0f, 45.0f, 0.0f);
    }
    
    void rotate(float dx, float dy) {
        // Professional 3D turntable navigation system
        const float sensitivity = 0.007f; // Optimized mouse sensitivity for smooth navigation
        
        // Convert to radians
        float angle_x = dx * sensitivity; // Horizontal mouse delta  
        float angle_y = dy * sensitivity; // Vertical mouse delta
        
        // Standard turntable rotation using quaternions:
        // 1. Vertical tilt around camera's local X-axis (pitch)
        QVector3D x_axis = rotation_.rotatedVector(QVector3D(1, 0, 0));
        QQuaternion quat_local_x = QQuaternion::fromAxisAndAngle(x_axis, -angle_y * 57.2958f);
        QQuaternion temp_quat = quat_local_x * rotation_;
        
        // 2. Horizontal spin around global Y-axis (yaw)
        QQuaternion quat_global_y = QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), angle_x * 57.2958f);
        
        // 3. Combine rotations for smooth dual-axis navigation
        rotation_ = quat_global_y * temp_quat;
        
        // 4. Maintain quaternion precision
        rotation_.normalize();
    }
    
    void pan(float dx, float dy) {
        // Professional 3D pan with camera-relative movement
        float factor = distance_ * 0.005f; // Distance-proportional sensitivity for smooth panning
        
        // Get camera's right and up vectors from quaternion rotation
        QVector3D right = rotation_.rotatedVector(QVector3D(1, 0, 0)); // Camera right is +X
        QVector3D up = rotation_.rotatedVector(QVector3D(0, 1, 0)); // Camera up is +Y
        
        // Apply panning in camera-relative directions
        QVector3D pan_offset = (right * dx + up * dy) * factor;
        target_x_ += pan_offset.x();
        target_y_ += pan_offset.y();
        target_z_ += pan_offset.z();
    }
    
    void zoom(float delta) {
        // Professional 3D zoom with exponential scaling
        const float zoom_factor = 1.2f; // Smooth exponential zoom steps
        float multiplier = (delta > 0) ? zoom_factor : (1.0f / zoom_factor);
        distance_ *= multiplier;
        distance_ = qBound(0.1f, distance_, 5000.0f);
    }
    
    float get_distance() const { return distance_; }
    void set_distance(float distance) { distance_ = qBound(1.0f, distance, 5000.0f); }
    
    QMatrix4x4 get_view_matrix() const {
        // Use quaternion to calculate camera position and orientation
        QVector3D forward = rotation_.rotatedVector(QVector3D(0, 0, -1)); // Camera looks down -Z
        QVector3D up = rotation_.rotatedVector(QVector3D(0, 1, 0)); // Camera up is +Y
        
        // Calculate camera world position (distance away from target, in forward direction)
        QVector3D camera_pos = QVector3D(target_x_, target_y_, target_z_) - forward * distance_;
        
        QMatrix4x4 view;
        view.lookAt(camera_pos, 
                   QVector3D(target_x_, target_y_, target_z_), 
                   up);
        return view;
    }
    
    QMatrix4x4 get_projection_matrix(float aspect) const {
        QMatrix4x4 proj;
        proj.perspective(45.0f, aspect, 0.1f, 10000.0f); // Much farther far plane
        return proj;
    }
};

// Professional OpenGL viewport following Blender's architecture
class VoxelOpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT

public:
    explicit VoxelOpenGLWidget(QWidget *parent = nullptr) : QOpenGLWidget(parent) {
        // Enable gesture recognition for trackpad/smart mouse
        grabGesture(Qt::PanGesture);
        grabGesture(Qt::PinchGesture);
        
        // Enable mouse tracking for smart mouse detection
        setMouseTracking(true);
        
        // Set up for native gesture events (macOS)
        setAttribute(Qt::WA_AcceptTouchEvents, true);
        
        // Detect system scroll direction preference (Blender-style)
        detectScrollDirection();
    }

private:
    // GPU Resources (Blender-style resource management)
    QOpenGLVertexArrayObject vao_;
    QOpenGLBuffer vertex_buffer_;
    QOpenGLShaderProgram shader_program_;
    
    // Camera system
    VoxelCamera camera_;
    
    // Navigation state
    bool mouse_pressed_ = false;
    int pressed_button_ = 0;
    QPoint last_mouse_pos_;
    
    // Grid geometry
    std::vector<float> grid_vertices_;
    int vertex_count_ = 0;
    
    // Viewport widget system
    QOpenGLVertexArrayObject widget_vao_;
    QOpenGLBuffer widget_vertex_buffer_;
    QOpenGLBuffer widget_vbo_;
    QOpenGLShaderProgram widget_shader_program_;
    std::vector<float> widget_vertices_;
    int widget_vertex_count_ = 0;
    float widget_center_x_ = 0.0f;
    float widget_center_y_ = 0.0f;
    float widget_radius_ = 0.0f;
    
    // Theme system handles all colors and styling
    
    // Sphere data for text rendering and hover detection
    struct AxisSphere {
        float pos[3];
        float color[3];  // Store colors directly instead of pointer
        bool is_positive;
        char label;
        int axis_index;
        float depth;
    };
    std::vector<AxisSphere> sphere_data_;
    int hovered_sphere_ = -1;
    
    // Gesture tracking
    float gesture_start_distance_ = 0.0f;
    QPointF gesture_start_pos_;
    
    // Scroll direction handling
    bool natural_scroll_direction_ = false;
    
    void render_debug_widget() {
        // Working navigation widget using widget shader
        GLboolean depth_test_enabled = glIsEnabled(GL_DEPTH_TEST);
        GLboolean blend_enabled = glIsEnabled(GL_BLEND);
        
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        widget_shader_program_.bind();
        
        // Widget positioning in bottom-right
        const float widget_size = 80.0f;
        const float margin = 30.0f;
        float widget_center_x = width() - margin - widget_size/2;
        float widget_center_y = margin + widget_size/2;
        
        // Set up matrices similar to successful test
        QMatrix4x4 widget_model, widget_view, widget_projection;
        
        // Screen-space orthographic 
        widget_projection.ortho(0.0f, float(width()), 0.0f, float(height()), -100.0f, 100.0f);
        
        // Position and scale widget
        widget_view.translate(widget_center_x, widget_center_y, 0.0f);
        widget_view.scale(widget_size/2.0f);  // Scale to fit
        
        // Apply camera rotation to widget
        QMatrix4x4 camera_view = camera_.get_view_matrix();
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                widget_model(i, j) = camera_view(i, j);
            }
        }
        widget_model(3, 3) = 1.0f;
        
        widget_shader_program_.setUniformValue("model", widget_model);
        widget_shader_program_.setUniformValue("view", widget_view);
        widget_shader_program_.setUniformValue("projection", widget_projection);
        
        // Generate sphere and line geometry like Blender
        std::vector<float> widget_verts;
        const float axis_distance = 1.0f;  // Further apart like Blender
        const float sphere_size = 0.12f;   // Circle size
        const float line_width = 0.05f;    // A hair thicker tubes
        
        // Helper to add a proper 3D cylindrical line (like Blender)
        auto add_cylindrical_line = [&](float x1, float y1, float z1, float x2, float y2, float z2, float r, float g, float b, float a) {
            float radius = line_width / 2.0f;
            int segments = 8; // 8-sided cylinder
            
            // Calculate cylinder direction vector
            float dx = x2 - x1, dy = y2 - y1, dz = z2 - z1;
            float length = sqrt(dx*dx + dy*dy + dz*dz);
            if (length < 0.001f) return;
            
            dx /= length; dy /= length; dz /= length;
            
            // Find perpendicular vectors for cylinder cross-section
            float ux = (abs(dx) < 0.9f) ? 1.0f : 0.0f;
            float uy = (abs(dy) < 0.9f) ? 1.0f : 0.0f; 
            float uz = (abs(dz) < 0.9f) ? 1.0f : 0.0f;
            
            // Cross product to get perpendicular
            float px = dy * uz - dz * uy;
            float py = dz * ux - dx * uz;
            float pz = dx * uy - dy * ux;
            float plen = sqrt(px*px + py*py + pz*pz);
            px /= plen; py /= plen; pz /= plen;
            
            // Another perpendicular
            float qx = dy * pz - dz * py;
            float qy = dz * px - dx * pz;
            float qz = dx * py - dy * px;
            
            // Generate cylinder triangles
            for (int i = 0; i < segments; i++) {
                float angle1 = 2.0f * M_PI * i / segments;
                float angle2 = 2.0f * M_PI * (i + 1) / segments;
                
                float c1 = cos(angle1) * radius, s1 = sin(angle1) * radius;
                float c2 = cos(angle2) * radius, s2 = sin(angle2) * radius;
                
                // Side quad (2 triangles)
                float v1x = x1 + px * c1 + qx * s1, v1y = y1 + py * c1 + qy * s1, v1z = z1 + pz * c1 + qz * s1;
                float v2x = x2 + px * c1 + qx * s1, v2y = y2 + py * c1 + qy * s1, v2z = z2 + pz * c1 + qz * s1;
                float v3x = x2 + px * c2 + qx * s2, v3y = y2 + py * c2 + qy * s2, v3z = z2 + pz * c2 + qz * s2;
                float v4x = x1 + px * c2 + qx * s2, v4y = y1 + py * c2 + qy * s2, v4z = z1 + pz * c2 + qz * s2;
                
                // Triangle 1
                widget_verts.insert(widget_verts.end(), {
                    v1x, v1y, v1z, r, g, b, a,
                    v2x, v2y, v2z, r, g, b, a,
                    v3x, v3y, v3z, r, g, b, a,
                });
                
                // Triangle 2
                widget_verts.insert(widget_verts.end(), {
                    v1x, v1y, v1z, r, g, b, a,
                    v3x, v3y, v3z, r, g, b, a,
                    v4x, v4y, v4z, r, g, b, a,
                });
            }
        };
        
        // Helper to add a circular sphere (like Blender)
        auto add_circle_sphere = [&](float x, float y, float z, float r, float g, float b, float a, bool filled = true) {
            int segments = 16; // Smooth circles
            float radius = sphere_size;
            
            if (filled) {
                // Filled circle for positive axes
                for (int i = 0; i < segments; i++) {
                    float angle1 = 2.0f * M_PI * i / segments;
                    float angle2 = 2.0f * M_PI * (i + 1) / segments;
                    
                    // Triangle fan from center
                    widget_verts.insert(widget_verts.end(), {
                        x, y, z, r, g, b, a, // Center
                        x + radius * cos(angle1), y + radius * sin(angle1), z, r, g, b, a,
                        x + radius * cos(angle2), y + radius * sin(angle2), z, r, g, b, a,
                    });
                }
            } else {
                // Hollow circle for negative axes (ring)
                float inner_radius = radius * 0.6f;
                for (int i = 0; i < segments; i++) {
                    float angle1 = 2.0f * M_PI * i / segments;
                    float angle2 = 2.0f * M_PI * (i + 1) / segments;
                    
                    float c1 = cos(angle1), s1 = sin(angle1);
                    float c2 = cos(angle2), s2 = sin(angle2);
                    
                    // Outer triangle
                    widget_verts.insert(widget_verts.end(), {
                        x + radius * c1, y + radius * s1, z, r, g, b, a,
                        x + radius * c2, y + radius * s2, z, r, g, b, a,
                        x + inner_radius * c1, y + inner_radius * s1, z, r, g, b, a,
                    });
                    
                    // Inner triangle
                    widget_verts.insert(widget_verts.end(), {
                        x + radius * c2, y + radius * s2, z, r, g, b, a,
                        x + inner_radius * c2, y + inner_radius * s2, z, r, g, b, a,
                        x + inner_radius * c1, y + inner_radius * s1, z, r, g, b, a,
                    });
                }
            }
        };
        
        // Add true 3D cylindrical lines from center to positive axes only (like Blender)
        float line_end = axis_distance - sphere_size; // End before spheres
        
        // Your preferred colors: #F54250 (red), #69C206 (green), #3987DB (blue)
        float red_r = 0.96f, red_g = 0.26f, red_b = 0.31f;     // #F54250
        float green_r = 0.41f, green_g = 0.76f, green_b = 0.02f; // #69C206  
        float blue_r = 0.22f, blue_g = 0.53f, blue_b = 0.86f;   // #3987DB
        
        add_cylindrical_line(0.0f, 0.0f, 0.0f, line_end, 0.0f, 0.0f, red_r, red_g, red_b, 0.8f);      // X line
        add_cylindrical_line(0.0f, 0.0f, 0.0f, 0.0f, line_end, 0.0f, green_r, green_g, green_b, 0.8f); // Y line  
        add_cylindrical_line(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, line_end, blue_r, blue_g, blue_b, 0.8f);    // Z line
        
        // Add circles for each axis (filled for positive, hollow for negative)
        add_circle_sphere(axis_distance, 0.0f, 0.0f, red_r, red_g, red_b, 0.9f, true);      // +X red filled
        add_circle_sphere(-axis_distance, 0.0f, 0.0f, red_r, red_g, red_b, 0.8f, false);    // -X red hollow
        add_circle_sphere(0.0f, axis_distance, 0.0f, green_r, green_g, green_b, 0.9f, true); // +Y green filled
        add_circle_sphere(0.0f, -axis_distance, 0.0f, green_r, green_g, green_b, 0.8f, false); // -Y green hollow
        add_circle_sphere(0.0f, 0.0f, axis_distance, blue_r, blue_g, blue_b, 0.9f, true);    // +Z blue filled
        add_circle_sphere(0.0f, 0.0f, -axis_distance, blue_r, blue_g, blue_b, 0.8f, false);  // -Z blue hollow
        
        // Render all widget geometry (lines + spheres)
        QOpenGLVertexArrayObject widget_vao;
        QOpenGLBuffer widget_vbo;
        widget_vao.create();
        widget_vao.bind();
        widget_vbo.create();
        widget_vbo.bind();
        widget_vbo.allocate(widget_verts.data(), widget_verts.size() * sizeof(float));
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glDrawArrays(GL_TRIANGLES, 0, widget_verts.size() / 7);
        
        widget_vbo.release();
        widget_vao.release();
        widget_shader_program_.release();
        
        // Restore GL state
        if (depth_test_enabled) glEnable(GL_DEPTH_TEST);
        if (!blend_enabled) glDisable(GL_BLEND);
    }

    void render_simple_navigation_widget() {
        // Simple modern OpenGL widget that won't interfere with grid
        // Save GL state
        GLboolean depth_test_enabled = glIsEnabled(GL_DEPTH_TEST);
        GLboolean blend_enabled = glIsEnabled(GL_BLEND);
        
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Use the same shader as the grid to avoid state conflicts
        shader_program_.bind();
        
        // Simple widget in bottom-right corner
        const float widget_size = 60.0f;
        const float margin = 20.0f;
        float widget_x = width() - margin - widget_size;
        float widget_y = margin;
        
        // 2D orthographic setup
        QMatrix4x4 widget_model, widget_view, widget_projection;
        widget_projection.ortho(0.0f, float(width()), 0.0f, float(height()), -1.0f, 1.0f);
        
        // Position widget
        widget_view.translate(widget_x + widget_size/2, widget_y + widget_size/2, 0);
        
        // Apply camera rotation  
        QMatrix4x4 camera_view = camera_.get_view_matrix();
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                widget_model(i, j) = camera_view(i, j);
            }
        }
        widget_model(3, 3) = 1.0f;
        
        // Set uniforms (same as grid shader)
        shader_program_.setUniformValue("model", widget_model);
        shader_program_.setUniformValue("view", widget_view);
        shader_program_.setUniformValue("projection", widget_projection);
        
        // Create simple quad vertices for each axis
        const float axis_distance = 20.0f;
        const float quad_size = 8.0f;
        
        // Generate simple quad geometry
        std::vector<float> widget_verts;
        
        // +X axis (red quad)
        float red_color[] = {0.9f, 0.2f, 0.2f, 0.9f};
        float x_pos[] = {axis_distance, 0.0f, 0.0f};
        for (int i = 0; i < 6; i++) { // 2 triangles = 6 vertices
            float vx[] = {-quad_size/2, quad_size/2, quad_size/2, -quad_size/2, quad_size/2, -quad_size/2};
            float vy[] = {-quad_size/2, -quad_size/2, quad_size/2, -quad_size/2, quad_size/2, quad_size/2};
            widget_verts.push_back(x_pos[0] + vx[i]); // X
            widget_verts.push_back(x_pos[1] + vy[i]); // Y  
            widget_verts.push_back(x_pos[2]); // Z
            widget_verts.push_back(red_color[0]); // R
            widget_verts.push_back(red_color[1]); // G
            widget_verts.push_back(red_color[2]); // B
            widget_verts.push_back(red_color[3]); // A
        }
        
        // Create temporary VAO/VBO
        QOpenGLVertexArrayObject temp_vao;
        QOpenGLBuffer temp_vbo;
        temp_vao.create();
        temp_vao.bind();
        temp_vbo.create();
        temp_vbo.bind();
        temp_vbo.allocate(widget_verts.data(), widget_verts.size() * sizeof(float));
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        temp_vbo.release();
        temp_vao.release();
        
        shader_program_.release();
        
        // Restore GL state
        if (depth_test_enabled) glEnable(GL_DEPTH_TEST);
        if (!blend_enabled) glDisable(GL_BLEND);
    }

    void render_viewport_widget() {
        // Skip rendering if widget isn't properly initialized
        if (widget_vertex_count_ <= 0 || !widget_vao_.isCreated()) {
            return;
        }
        
        // Save current GL state
        GLboolean depth_test_enabled = glIsEnabled(GL_DEPTH_TEST);
        GLboolean blend_enabled = glIsEnabled(GL_BLEND);
        
        // Disable depth test for widget overlay
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Set up widget rendering
        widget_shader_program_.bind();
        
        // Create widget matrices
        QMatrix4x4 widget_model;
        QMatrix4x4 widget_view;
        QMatrix4x4 widget_projection;
        
        // Widget positioning and size (larger for better visibility)
        const float widget_size = 80.0f; // Larger size like Blender
        const float margin = 35.0f; // Margin from edges
        widget_center_x_ = float(width()) - margin - widget_size/2;
        widget_center_y_ = margin + widget_size/2;
        widget_radius_ = widget_size/2.0f;
        
        // Screen-space orthographic projection
        widget_projection.ortho(0.0f, float(width()), 0.0f, float(height()), -100.0f, 100.0f);
        
        
        // Position the widget
        widget_view.translate(widget_center_x_, widget_center_y_, 0.0f);
        widget_view.scale(widget_radius_);
        
        // Apply camera rotation - but we want orthographic rendering
        QMatrix4x4 camera_view = camera_.get_view_matrix();
        
        // Extract just the rotation part for the widget
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                widget_model(i, j) = camera_view(i, j);
            }
        }
        widget_model(3, 3) = 1.0f;
        
        
        // Set uniforms for main widget
        widget_shader_program_.setUniformValue("model", widget_model);
        widget_shader_program_.setUniformValue("view", widget_view);
        widget_shader_program_.setUniformValue("projection", widget_projection);
        
        // Calculate depth for each sphere using camera matrix for sorting
        float min_depth = FLT_MAX, max_depth = -FLT_MAX;
        for (int i = 0; i < sphere_data_.size(); i++) {
            AxisSphere& sphere = sphere_data_[i];
            // Transform sphere position by camera view matrix to get depth (Z component)
            QVector4D pos(sphere.pos[0], sphere.pos[1], sphere.pos[2], 1.0f);
            QVector4D transformed = camera_view * pos;
            sphere.depth = transformed.z(); // Raw depth
            min_depth = qMin(min_depth, sphere.depth);
            max_depth = qMax(max_depth, sphere.depth);
        }
        
        // Normalize depth values to [-1, 1] range like Blender expects
        float depth_range = max_depth - min_depth;
        if (depth_range > 0.0f) {
            for (int i = 0; i < sphere_data_.size(); i++) {
                AxisSphere& sphere = sphere_data_[i];
                // Normalize to [-1, 1] range
                sphere.depth = 2.0f * ((sphere.depth - min_depth) / depth_range) - 1.0f;
            }
        } else {
            // All spheres at same depth
            for (int i = 0; i < sphere_data_.size(); i++) {
                sphere_data_[i].depth = 0.0f;
            }
        }
        
        // Sort spheres by depth (back-to-front rendering for proper alpha blending)
        std::sort(sphere_data_.begin(), sphere_data_.end(), [](const AxisSphere& a, const AxisSphere& b) {
            return a.depth < b.depth; // Render furthest first (smallest Z)
        });
        
        // Regenerate vertex buffer with sorted spheres
        regenerate_sorted_widget(camera_view);
        
        // Render main widget
        widget_vao_.bind();
        if (widget_vertex_count_ > 0) {
            // Reset buffer to widget data
            widget_vbo_.bind();
            glBufferSubData(GL_ARRAY_BUFFER, 0, widget_vertices_.size() * sizeof(float), widget_vertices_.data());
            widget_vbo_.release();
            
            glDrawArrays(GL_TRIANGLES, 0, widget_vertex_count_);
        }
        widget_vao_.release();
        
        widget_shader_program_.release();
        
        // TODO: Add OpenGL-based text rendering instead of QPainter to avoid state corruption
        // render_sphere_labels(widget_projection, widget_view, widget_model);
        
        // Restore GL state
        if (depth_test_enabled) glEnable(GL_DEPTH_TEST);
        if (!blend_enabled) glDisable(GL_BLEND);
    }
    
    void render_sphere_labels(const QMatrix4x4& projection, const QMatrix4x4& view, const QMatrix4x4& model) {
        // Simple text rendering using QPainter overlay (basic implementation)
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // Calculate MVP matrix
        QMatrix4x4 mvp = projection * view * model; 
        
        for (int i = 0; i < sphere_data_.size(); i++) {
            const AxisSphere& sphere = sphere_data_[i];
            
            // Determine if this sphere should show text
            bool show_text = sphere.is_positive; // Only show text on positive axes for now
            QColor text_color = Qt::black;
            
            if (show_text) {
                // Project sphere position to screen
                QVector4D sphere_pos(sphere.pos[0], sphere.pos[1], sphere.pos[2], 1.0f);
                QVector4D screen_pos = mvp * sphere_pos;
                
                if (screen_pos.w() > 0) {
                    screen_pos /= screen_pos.w();
                    int x = (screen_pos.x() + 1.0f) * 0.5f * width();
                    int y = (1.0f - screen_pos.y()) * 0.5f * height();
                    
                    // Prepare text
                    QString label;
                    if (sphere.is_positive) {
                        label = QString(sphere.label);
                    } else {
                        label = QString("-%1").arg(sphere.label);
                    }
                    
                    // Set font and color
                    QFont font = painter.font();
                    font.setPointSize(10);
                    font.setBold(true);
                    painter.setFont(font);
                    painter.setPen(text_color);
                    
                    // Draw text centered on sphere
                    QFontMetrics fm(font);
                    int text_width = fm.horizontalAdvance(label);
                    int text_height = fm.height();
                    
                    painter.drawText(x - text_width/2, y + text_height/4, label);
                }
            }
        }
        
        painter.end();
    }
    

    void generate_viewport_widget() {
        widget_vertices_.clear();
        
        // Navigation widget with spheres and cylindrical lines (Blender approach)
        const float axis_distance = 0.85f;  // Distance from center to spheres
        const float sphere_radius = 0.18f;  // Sphere size
        const float line_radius = 0.025f;   // Slightly thicker cylindrical lines
        const int circle_segments = 32;     // Smooth circles
        
        // Get axis colors from theme system
        const auto& x_color = Colors::XAxis();
        const auto& y_color = Colors::YAxis();
        const auto& z_color = Colors::ZAxis();
        
        // Generate cylindrical lines from center to positive axes only (like Blender)
        const float line_end_offset = sphere_radius * 0.1f; // Small offset so line touches sphere
        
        // Lines meet exactly at center (0,0,0) and extend to spheres
        // +X axis line (center to +X)
        generate_cylindrical_line(0.0f, 0.0f, 0.0f, 
                                 axis_distance - line_end_offset, 0.0f, 0.0f,
                                 x_color.data(), Theme::instance().alpha().line, line_radius);
        // +Y axis line (center to +Y)
        generate_cylindrical_line(0.0f, 0.0f, 0.0f,
                                 0.0f, axis_distance - line_end_offset, 0.0f,
                                 y_color.data(), Theme::instance().alpha().line, line_radius);
        // +Z axis line (center to +Z)
        generate_cylindrical_line(0.0f, 0.0f, 0.0f,
                                 0.0f, 0.0f, axis_distance - line_end_offset,
                                 z_color.data(), Theme::instance().alpha().line, line_radius);
        
        // Note: Depth sorting will be done in render function where camera is accessible
        
        // Store sphere data for text rendering and hover detection
        sphere_data_.clear();
        sphere_data_.resize(6);
        
        // Populate sphere data
        AxisSphere& sphere0 = sphere_data_[0];
        sphere0.pos[0] = axis_distance; sphere0.pos[1] = 0.0f; sphere0.pos[2] = 0.0f;
        sphere0.color[0] = x_color[0]; sphere0.color[1] = x_color[1]; sphere0.color[2] = x_color[2];
        sphere0.is_positive = true; sphere0.label = 'X'; sphere0.axis_index = 0; sphere0.depth = 0.0f;
        
        AxisSphere& sphere1 = sphere_data_[1];  
        sphere1.pos[0] = -axis_distance; sphere1.pos[1] = 0.0f; sphere1.pos[2] = 0.0f;
        sphere1.color[0] = x_color[0]; sphere1.color[1] = x_color[1]; sphere1.color[2] = x_color[2];
        sphere1.is_positive = false; sphere1.label = 'X'; sphere1.axis_index = 0; sphere1.depth = 0.0f;
        
        AxisSphere& sphere2 = sphere_data_[2];
        sphere2.pos[0] = 0.0f; sphere2.pos[1] = axis_distance; sphere2.pos[2] = 0.0f;
        sphere2.color[0] = y_color[0]; sphere2.color[1] = y_color[1]; sphere2.color[2] = y_color[2];
        sphere2.is_positive = true; sphere2.label = 'Y'; sphere2.axis_index = 1; sphere2.depth = 0.0f;
        
        AxisSphere& sphere3 = sphere_data_[3];
        sphere3.pos[0] = 0.0f; sphere3.pos[1] = -axis_distance; sphere3.pos[2] = 0.0f;
        sphere3.color[0] = y_color[0]; sphere3.color[1] = y_color[1]; sphere3.color[2] = y_color[2];
        sphere3.is_positive = false; sphere3.label = 'Y'; sphere3.axis_index = 1; sphere3.depth = 0.0f;
        
        AxisSphere& sphere4 = sphere_data_[4];
        sphere4.pos[0] = 0.0f; sphere4.pos[1] = 0.0f; sphere4.pos[2] = axis_distance;
        sphere4.color[0] = z_color[0]; sphere4.color[1] = z_color[1]; sphere4.color[2] = z_color[2];
        sphere4.is_positive = true; sphere4.label = 'Z'; sphere4.axis_index = 2; sphere4.depth = 0.0f;
        
        AxisSphere& sphere5 = sphere_data_[5];
        sphere5.pos[0] = 0.0f; sphere5.pos[1] = 0.0f; sphere5.pos[2] = -axis_distance;
        sphere5.color[0] = z_color[0]; sphere5.color[1] = z_color[1]; sphere5.color[2] = z_color[2];
        sphere5.is_positive = false; sphere5.label = 'Z'; sphere5.axis_index = 2; sphere5.depth = 0.0f;
        
        // Generate 3D spheres
        for (int i = 0; i < 6; i++) {
            const AxisSphere& sphere = sphere_data_[i];
            
            if (sphere.is_positive) {
                // Solid sphere for positive axes
                generate_sphere(sphere.pos[0], sphere.pos[1], sphere.pos[2],
                               sphere_radius, sphere.color, Colors::PositiveSphere(), 16, 16);
            } else {
                // Hollow sphere for negative axes with lower opacity
                generate_sphere_outline(sphere.pos[0], sphere.pos[1], sphere.pos[2],
                                       sphere_radius, sphere.color, Colors::NegativeFill(), 12, 12, 0.03f);
            }
        }
        
        // Store vertex count
        widget_vertex_count_ = widget_vertices_.size() / 7;
        qDebug() << "Generated navigation widget with" << widget_vertex_count_ << "vertices";
    }
    
    void regenerate_sorted_widget(const QMatrix4x4& camera_view) {
        // Clear existing vertex data
        widget_vertices_.clear();
        
        // Navigation widget parameters matching generate_viewport_widget()
        const float axis_distance = 0.85f;
        const float sphere_radius = 0.18f; // Sphere size
        const float line_radius = 0.025f;
        
        // Get axis colors from theme system
        const auto& x_color = Colors::XAxis();
        const auto& y_color = Colors::YAxis();
        const auto& z_color = Colors::ZAxis();
        
        // Extract camera forward direction from view matrix
        QVector3D camera_forward = -QVector3D(camera_view(0, 2), camera_view(1, 2), camera_view(2, 2)).normalized();
        
        // Generate cylindrical lines from center to positive axes only (same as original)
        const float line_end_offset = sphere_radius * 0.1f; // Small offset so line touches sphere
        
        // +X axis line
        generate_cylindrical_line(0.0f, 0.0f, 0.0f, 
                                 axis_distance - line_end_offset, 0.0f, 0.0f,
                                 x_color.data(), Theme::instance().alpha().line, line_radius);
        // +Y axis line
        generate_cylindrical_line(0.0f, 0.0f, 0.0f,
                                 0.0f, axis_distance - line_end_offset, 0.0f,
                                 y_color.data(), Theme::instance().alpha().line, line_radius);
        // +Z axis line
        generate_cylindrical_line(0.0f, 0.0f, 0.0f,
                                 0.0f, 0.0f, axis_distance - line_end_offset,
                                 z_color.data(), Theme::instance().alpha().line, line_radius);
        
        // Generate spheres in sorted order (back-to-front for proper transparency)
        for (const AxisSphere& sphere : sphere_data_) {
            if (sphere.is_positive) {
                // Solid sphere for positive axes
                generate_sphere(sphere.pos[0], sphere.pos[1], sphere.pos[2],
                               sphere_radius, sphere.color, Colors::PositiveSphere(), 16, 16);
            } else {
                // Hollow sphere for negative axes with lower fill opacity and visible outline
                generate_sphere_outline(sphere.pos[0], sphere.pos[1], sphere.pos[2],
                                       sphere_radius, sphere.color, Colors::NegativeFill(), 12, 12, 0.03f, camera_forward);
            }
        }
        
        // Update vertex count
        widget_vertex_count_ = widget_vertices_.size() / 7;
    }
    
    void generate_cylindrical_line(float x1, float y1, float z1, float x2, float y2, float z2,
                                  const float color[3], float alpha, float radius) {
        // Create a proper 3D cylinder for the line
        QVector3D start(x1, y1, z1);
        QVector3D end(x2, y2, z2);
        QVector3D dir = (end - start).normalized();
        float length = (end - start).length();
        
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
    
    void generate_sphere(float cx, float cy, float cz, float radius,
                        const float color[3], float alpha, int lat_segments, int lon_segments) {
        // Generate solid 3D sphere
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
                QVector3D v2(cx + radius * sin(phi1) * cos(theta2),
                            cy + radius * cos(phi1),
                            cz + radius * sin(phi1) * sin(theta2));
                QVector3D v3(cx + radius * sin(phi2) * cos(theta1),
                            cy + radius * cos(phi2),
                            cz + radius * sin(phi2) * sin(theta1));
                QVector3D v4(cx + radius * sin(phi2) * cos(theta2),
                            cy + radius * cos(phi2),
                            cz + radius * sin(phi2) * sin(theta2));
                
                // Two triangles per quad
                widget_vertices_.insert(widget_vertices_.end(), {
                    v1.x(), v1.y(), v1.z(), color[0], color[1], color[2], alpha,
                    v3.x(), v3.y(), v3.z(), color[0], color[1], color[2], alpha,
                    v2.x(), v2.y(), v2.z(), color[0], color[1], color[2], alpha,
                    
                    v2.x(), v2.y(), v2.z(), color[0], color[1], color[2], alpha,
                    v3.x(), v3.y(), v3.z(), color[0], color[1], color[2], alpha,
                    v4.x(), v4.y(), v4.z(), color[0], color[1], color[2], alpha
                });
            }
        }
    }
    
    void generate_sphere_outline(float cx, float cy, float cz, float radius,
                                const float color[3], float alpha, int lat_segments, int lon_segments, float thickness,
                                const QVector3D& camera_forward = QVector3D(0, 0, -1)) {
        // Blender's exact approach: single sphere with mixed inner_color and outline_color
        // They use: interp_v4_v4v4(negative_color, view_color, axis_color[axis], 0.25f)
        
        const auto& bg_color = Theme::instance().ui().background;
        float mixed_color[3] = {
            bg_color[0] * 0.75f + color[0] * 0.25f,  // 75% background + 25% axis color
            bg_color[1] * 0.75f + color[1] * 0.25f,
            bg_color[2] * 0.75f + color[2] * 0.25f
        };
        
        // Create simple outline-only sphere - user requested "just the outline but like a sphere"
        // Use low-resolution sphere to create clean outline appearance
        generate_sphere(cx, cy, cz, radius, mixed_color, 0.4f, 8, 6);
    }
    
    void generate_blender_infinite_grid() {
        grid_vertices_.clear();
        
        // Blender uses a much larger tessellated grid for infinite appearance
        constexpr int resolution = 32; // Higher resolution for better quality
        std::vector<float> steps(resolution + 1);
        
        // [-1, 1] divided into "resolution" steps (Blender's exact approach)
        for (int i = 0; i <= resolution; i++) {
            steps[i] = -1.0f + float(i * 2) / resolution;
        }
        
        // Generate tessellated triangles (Blender's exact method)
        grid_vertices_.reserve(resolution * resolution * 6 * 7);
        
        for (int x = 0; x < resolution; x++) {
            for (int y = 0; y < resolution; y++) {
                // First triangle of quad
                grid_vertices_.insert(grid_vertices_.end(), {
                    steps[x], 0.0f, steps[y], 0.0f, 0.0f, 0.0f, 1.0f,       
                    steps[x + 1], 0.0f, steps[y], 0.0f, 0.0f, 0.0f, 1.0f,   
                    steps[x], 0.0f, steps[y + 1], 0.0f, 0.0f, 0.0f, 1.0f    
                });
                
                // Second triangle of quad
                grid_vertices_.insert(grid_vertices_.end(), {
                    steps[x], 0.0f, steps[y + 1], 0.0f, 0.0f, 0.0f, 1.0f,   
                    steps[x + 1], 0.0f, steps[y], 0.0f, 0.0f, 0.0f, 1.0f,   
                    steps[x + 1], 0.0f, steps[y + 1], 0.0f, 0.0f, 0.0f, 1.0f 
                });
            }
        }
        
        vertex_count_ = resolution * resolution * 6;
        qDebug() << "Generated grid with" << vertex_count_ << "vertices";
        qDebug() << "First vertex position:" << grid_vertices_[0] << grid_vertices_[1] << grid_vertices_[2];
    }
    
    void detectScrollDirection() {
        // Detect system scroll direction preference (like Blender)
        natural_scroll_direction_ = false;
        
#ifdef Q_OS_MACOS
        // Check macOS "Natural Scrolling" preference
        QSettings settings("~/Library/Preferences/.GlobalPreferences.plist", QSettings::NativeFormat);
        if (settings.contains("com.apple.swipescrolldirection")) {
            natural_scroll_direction_ = settings.value("com.apple.swipescrolldirection").toBool();
        }
        
        qDebug() << "macOS Natural Scrolling detected:" << (natural_scroll_direction_ ? "ON" : "OFF");
#endif
    }
    
    // Smart mouse and trackpad detection (Blender-style)
    bool isTrackpadOrSmartMouse(QWheelEvent *event) {
        QPoint pixelDelta = event->pixelDelta();
        QPoint angleDelta = event->angleDelta();
        
        // Trackpad/smart mouse typically provides pixel deltas
        // Regular mouse wheel provides only angle deltas
        return !pixelDelta.isNull() && (qAbs(pixelDelta.x()) > 0 || qAbs(pixelDelta.y()) > 0);
    }
    
    void handleGestureWheel(const QPoint& pixelDelta, Qt::KeyboardModifiers modifiers) {
        // Apply scroll direction inversion if natural scrolling is DISABLED (like Blender)
        QPoint adjustedDelta = pixelDelta;
        if (!natural_scroll_direction_) {
            adjustedDelta.setY(-pixelDelta.y());
        }
        
        if (modifiers & Qt::ControlModifier) {
            // Ctrl+scroll = Zoom (Blender behavior)
            // Use Blender's continuous zoom formula: fac / 20.0f
            float zoomFactor = adjustedDelta.y() / 20.0f;
            if (zoomFactor != 0.0f) {
                // Apply zoom factor more smoothly
                float multiplier = 1.0f + (zoomFactor * 0.1f);
                float newDistance = camera_.get_distance() * multiplier;
                camera_.set_distance(newDistance);
            }
        } else if (modifiers & Qt::ShiftModifier) {
            // Shift+scroll = Pan (Blender behavior)
            // Use Blender's pan step values: 32px horizontal, 25px vertical
            float panX = -adjustedDelta.x() * (32.0f / 100.0f); // Scale to reasonable values  
            float panY = adjustedDelta.y() * (25.0f / 100.0f);
            camera_.pan(panX, panY);
        } else {
            // Default scroll = Rotate (like Blender trackpad rotation)
            // Use the same Blender sensitivity as rotate()
            camera_.rotate(-adjustedDelta.x(), -adjustedDelta.y());
        }
        update();
    }
    
    void handleTraditionalWheel(const QPoint& angleDelta, Qt::KeyboardModifiers modifiers) {
        // Traditional mouse wheel (discrete steps) - Blender-exact behavior
        // Note: Traditional mouse wheel typically doesn't need scroll direction inversion
        if (modifiers & Qt::ShiftModifier) {
            // Shift + wheel = pan (using Blender's pan step values)
            if (angleDelta.x() != 0) {
                float panSteps = angleDelta.x() / 120.0f; // Convert to wheel steps
                camera_.pan(-panSteps * 32.0f, 0.0f); // 32px horizontal step like Blender
            }
            if (angleDelta.y() != 0) {
                float panSteps = angleDelta.y() / 120.0f;
                camera_.pan(0.0f, -panSteps * 25.0f); // 25px vertical step like Blender
            }
        } else {
            // Normal wheel = zoom (using Blender's 1.2x factor)
            if (angleDelta.y() != 0) {
                float wheelSteps = angleDelta.y() / 120.0f; // Convert to wheel steps
                // Apply Blender's zoom step for each wheel notch
                for (int i = 0; i < qAbs(wheelSteps); i++) {
                    camera_.zoom(wheelSteps > 0 ? 1 : -1);
                }
            }
        }
        update();
    }

protected:
    void initializeGL() override {
        qDebug() << "Initializing OpenGL...";
        
        // Initialize theme system
        Theme::instance().loadFromFile("assets/styles/colors.json");
        
        if (!initializeOpenGLFunctions()) {
            qFatal("Failed to initialize OpenGL functions");
            return;
        }
        
        // Log OpenGL info
        qDebug() << "OpenGL Version:" << (const char*)glGetString(GL_VERSION);
        qDebug() << "OpenGL Vendor:" << (const char*)glGetString(GL_VENDOR);
        qDebug() << "OpenGL Renderer:" << (const char*)glGetString(GL_RENDERER);
        
        // Set up OpenGL state (Blender-style)
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.168f, 0.168f, 0.168f, 1.0f); // Blender's background color
        
        // Generate initial grid geometry
        generate_blender_infinite_grid();
        
        // Generate viewport widget geometry
        generate_viewport_widget();
        
        // Create and compile shaders (OpenGL 3.3 Core) - Blender's exact approach
        const char* vertex_shader = R"(
            #version 330 core
            layout (location = 0) in vec3 position;
            layout (location = 1) in vec4 color;
            
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            uniform vec3 view_position;
            uniform float grid_size;
            uniform vec3 plane_axes;
            
            out vec3 local_pos;
            out vec3 world_pos;
            
            void main() {
                // Blender's exact vertex shader logic
                local_pos = position;
                
                // Simplified approach - position grid around origin
                vec3 real_pos = position * grid_size;
                real_pos.y = 0.0; // Grid on XZ plane
                world_pos = real_pos;
                
                gl_Position = projection * view * vec4(real_pos, 1.0);
            }
        )";
        
        const char* fragment_shader = R"(
            #version 330 core
            in vec3 local_pos;
            in vec3 world_pos;
            
            uniform vec3 view_position;
            uniform float grid_distance;
            uniform float line_size;
            uniform vec3 plane_axes;
            uniform mat4 view;
            
            // Grid colors (Blender-style)
            uniform vec3 grid_color;
            uniform vec3 grid_emphasis_color;
            uniform vec3 grid_axis_x_color;
            uniform vec3 grid_axis_z_color;
            
            out vec4 frag_color;
            
            #define LINE_STEP(x) smoothstep(0.0, 1.0, x)
            #define linearstep(p0, p1, v) (clamp(((v) - (p0)) / abs((p1) - (p0)), 0.0, 1.0))
            
            float get_grid(vec2 co, vec2 fwidthCos, float grid_scale) {
                float half_size = grid_scale / 2.0;
                // Triangular wave pattern, amplitude is [0, half_size]
                vec2 grid_domain = abs(mod(co + half_size, vec2(grid_scale)) - half_size);
                // Modulate by the absolute rate of change of the coordinates
                grid_domain /= fwidthCos;
                // Collapse waves
                float line_dist = min(grid_domain.x, grid_domain.y);
                return 1.0 - LINE_STEP(line_dist - line_size);
            }
            
            vec3 get_axes(vec3 co, vec3 fwidthCos, float axis_line_size) {
                vec3 axes_domain = abs(co);
                // Modulate by the absolute rate of change of the coordinates
                axes_domain /= fwidthCos;
                return 1.0 - LINE_STEP(axes_domain - (axis_line_size + line_size));
            }
            
            void main() {
                vec3 P = world_pos;
                vec3 dFdxPos = dFdx(P);
                vec3 dFdyPos = dFdy(P);
                vec3 fwidthPos = abs(dFdxPos) + abs(dFdyPos);
                
                // Grid calculations on XZ plane
                vec2 grid_pos = P.xz;
                vec2 grid_fwidth = fwidthPos.xz;
                
                // Grid resolution calculation - Blender's approach
                float grid_res = max(length(dFdxPos), length(dFdyPos));
                grid_res *= 4.0; // Grid begins to appear when it comprises 4 pixels
                
                // Multi-level grid scales for voxel editing
                float scaleA = 1.0;   // 1-unit grid (1 voxel)
                float scaleB = 10.0;  // 10-unit grid
                float scaleC = 100.0; // 100-unit grid
                
                // Blender's multi-level grid system with level-of-detail fading
                float blendA = 1.0 - linearstep(scaleA * 0.5, scaleA * 4.0, grid_res);
                float blendB = 1.0 - linearstep(scaleB * 0.5, scaleB * 4.0, grid_res);
                float blendC = 1.0 - linearstep(scaleC * 0.5, scaleC * 4.0, grid_res);
                
                // Apply cubic blending for smooth transitions
                blendA = blendA * blendA * blendA;
                blendB = blendB * blendB * blendB;
                blendC = blendC * blendC * blendC;
                
                float gridA = get_grid(grid_pos, grid_fwidth, scaleA);
                float gridB = get_grid(grid_pos, grid_fwidth, scaleB);
                float gridC = get_grid(grid_pos, grid_fwidth, scaleC);
                
                // Dynamic visibility based on zoom level (Blender's approach)
                vec3 color = grid_color;
                float alpha = 0.0;
                
                // 1-unit grid: becomes more visible as you zoom in close
                float alpha1 = gridA * blendA * (0.1 + blendA * 0.4); // Dynamic from 0.1 to 0.5
                
                // 10-unit grid: always slightly more visible than 1-unit
                float alpha10 = 0.0;
                if (blendB > 0.01) {
                    alpha10 = gridB * blendB * (0.3 + blendB * 0.3); // Dynamic from 0.3 to 0.6
                    color = mix(color, grid_emphasis_color, blendB * 0.5);
                }
                
                // 100-unit grid: most visible at far distances
                float alpha100 = 0.0;
                if (blendC > 0.01) {
                    alpha100 = gridC * blendC * (0.4 + blendC * 0.4); // Dynamic from 0.4 to 0.8
                    color = mix(color, grid_emphasis_color, blendC * 0.7);
                }
                
                // Combine alphas - higher level grids take precedence when visible
                alpha = max(alpha1, max(alpha10, alpha100));
                
                // Calculate axis lines with proper derivatives to keep them thin
                vec3 axes_dist = vec3(0.0);
                vec3 axes_fwidth = vec3(0.0);
                
                // X-axis (red line along Z direction)
                axes_dist.x = P.z;
                axes_fwidth.x = fwidthPos.z;
                
                // Z-axis (blue line along X direction)  
                axes_dist.z = P.x;
                axes_fwidth.z = fwidthPos.x;
                
                // Use thinner axis size to prevent thick lines up close
                vec3 axes = get_axes(axes_dist, axes_fwidth, 0.05); // Thinner axes
                
                // Apply axis colors and alpha
                if (axes.x > 1e-8) {
                    color = grid_axis_x_color;
                    alpha = max(alpha, axes.x);
                }
                if (axes.z > 1e-8) {
                    color = grid_axis_z_color;
                    alpha = max(alpha, axes.z);
                }
                
                // More gradual perspective fade calculation
                float fade = 1.0;
                vec3 V = view_position - P;
                float dist = length(V);
                V /= dist;
                
                // Gentler viewing angle fade - less abrupt than Blender's
                float angle = V.y;
                angle = 1.0 - abs(angle);
                angle = pow(angle, 1.5); // Less aggressive power curve
                fade = 1.0 - angle * 0.7; // Reduce fade strength from 1.0 to 0.7
                
                // More gradual distance fade - starts later and extends further
                fade *= 1.0 - smoothstep(grid_distance * 1.5, grid_distance * 3.0, dist);
                
                frag_color = vec4(color, alpha * fade);
            }
        )";
        
        // Compile shaders
        if (!shader_program_.addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader)) {
            qFatal("Failed to compile vertex shader: %s", qPrintable(shader_program_.log()));
        }
        
        if (!shader_program_.addShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader)) {
            qFatal("Failed to compile fragment shader: %s", qPrintable(shader_program_.log()));
        }
        
        if (!shader_program_.link()) {
            qFatal("Failed to link shader program: %s", qPrintable(shader_program_.log()));
        }
        
        // Set up VAO and VBO (Blender-style batch system)
        vao_.create();
        vao_.bind();
        
        vertex_buffer_.create();
        vertex_buffer_.bind();
        vertex_buffer_.allocate(grid_vertices_.data(), grid_vertices_.size() * sizeof(float));
        
        // Set up vertex attributes
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        vao_.release();
        vertex_buffer_.release();
        
        // Create widget shaders (simple vertex coloring)
        const char* widget_vertex_shader = R"(
            #version 330 core
            layout (location = 0) in vec3 position;
            layout (location = 1) in vec4 color;
            
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            
            out vec4 vertex_color;
            
            void main() {
                vertex_color = color;
                gl_Position = projection * view * model * vec4(position, 1.0);
            }
        )";
        
        const char* widget_fragment_shader = R"(
            #version 330 core
            in vec4 vertex_color;
            out vec4 frag_color;
            
            void main() {
                frag_color = vertex_color;
            }
        )";
        
        // Compile widget shaders
        if (!widget_shader_program_.addShaderFromSourceCode(QOpenGLShader::Vertex, widget_vertex_shader)) {
            qFatal("Failed to compile widget vertex shader: %s", qPrintable(widget_shader_program_.log()));
        }
        
        if (!widget_shader_program_.addShaderFromSourceCode(QOpenGLShader::Fragment, widget_fragment_shader)) {
            qFatal("Failed to compile widget fragment shader: %s", qPrintable(widget_shader_program_.log()));
        }
        
        if (!widget_shader_program_.link()) {
            qFatal("Failed to link widget shader program: %s", qPrintable(widget_shader_program_.log()));
        }
        
        // Set up widget VAO and VBO with error checking
        if (!widget_vertices_.empty() && widget_vertex_count_ > 0) {
            widget_vao_.create();
            widget_vao_.bind();
            
            widget_vbo_.create();
            widget_vbo_.bind();
            
            // Allocate buffer with size check
            int buffer_size = widget_vertices_.size() * sizeof(float);
            qDebug() << "Allocating widget buffer:" << buffer_size << "bytes for" << widget_vertex_count_ << "vertices";
            widget_vbo_.allocate(widget_vertices_.data(), buffer_size);
            
            // Set up widget vertex attributes (position + color)
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            
            widget_vao_.release();
            widget_vbo_.release();
            
            // Check for OpenGL errors after widget setup
            GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                qDebug() << "OpenGL Error after widget setup:" << err;
            }
        } else {
            qDebug() << "Warning: Empty widget vertices, skipping widget VAO setup";
        }
        
        qDebug() << "OpenGL viewport initialized with" << vertex_count_ << "vertices and" << widget_vertex_count_ << "widget vertices";
    }
    
    void paintGL() override {
        static int frame_count = 0;
        if (frame_count++ % 60 == 0) {
            float grid_size = qMax(2000.0f, camera_.get_distance() * 6.0f);
            QMatrix4x4 view = camera_.get_view_matrix();
            QVector3D view_pos = view.inverted().column(3).toVector3D();
            qDebug() << "Frame" << frame_count 
                     << "Camera distance:" << camera_.get_distance()
                     << "Grid size:" << grid_size
                     << "View pos:" << view_pos.x() << view_pos.y() << view_pos.z();
        }
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        shader_program_.bind();
        
        // Blender-style matrices
        QMatrix4x4 model;
        QMatrix4x4 view = camera_.get_view_matrix();
        QMatrix4x4 projection = camera_.get_projection_matrix(float(width()) / height());
        
        shader_program_.setUniformValue("model", model);
        shader_program_.setUniformValue("view", view);
        shader_program_.setUniformValue("projection", projection);
        
        // Blender's grid uniforms
        QVector3D view_position = view.inverted().column(3).toVector3D();
        float grid_distance = camera_.get_distance();
        float grid_size = qMax(2000.0f, grid_distance * 6.0f); // Huge grid coverage for far view
        
        shader_program_.setUniformValue("view_position", view_position);
        shader_program_.setUniformValue("grid_distance", grid_distance);
        shader_program_.setUniformValue("grid_size", grid_size);
        shader_program_.setUniformValue("line_size", 0.5f); // Blender's default line thickness
        
        // XZ plane configuration (for Minecraft-style Y-up world)
        shader_program_.setUniformValue("plane_axes", QVector3D(1.0f, 0.0f, 1.0f));
        
        // Balanced grid colors with better contrast
        shader_program_.setUniformValue("grid_color", QVector3D(0.25f, 0.25f, 0.25f)); // Slightly brighter base grid
        shader_program_.setUniformValue("grid_emphasis_color", QVector3D(0.4f, 0.4f, 0.4f)); // More visible emphasis grid
        shader_program_.setUniformValue("grid_axis_x_color", QVector3D(0.96f, 0.26f, 0.31f)); // #F54250 red X-axis
        shader_program_.setUniformValue("grid_axis_z_color", QVector3D(0.22f, 0.53f, 0.86f)); // #3987DB blue Z-axis
        
        // Draw tessellated triangles with Blender's grid shader
        vao_.bind();
        glDrawArrays(GL_TRIANGLES, 0, vertex_count_);
        vao_.release();
        
        shader_program_.release();
        
        // Render navigation widget with proper 3D spheres
        render_viewport_widget();
        
        // Check for OpenGL errors
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            qDebug() << "OpenGL Error:" << err;
        }
    }
    
    void resizeGL(int width, int height) override {
        glViewport(0, 0, width, height);
    }
    
    // Blender-style navigation controls
    void mousePressEvent(QMouseEvent *event) override {
        mouse_pressed_ = true;
        pressed_button_ = event->button();
        last_mouse_pos_ = event->pos();
    }
    
    void mouseReleaseEvent(QMouseEvent *event) override {
        mouse_pressed_ = false;
        pressed_button_ = 0;
    }
    
    void mouseMoveEvent(QMouseEvent *event) override {
        if (mouse_pressed_) {
            QPoint delta = event->pos() - last_mouse_pos_;
            
            if (delta.manhattanLength() > 0) {
                // Blender-style navigation with MMB alternatives
                bool has_shift = (event->modifiers() & Qt::ShiftModifier);
                bool has_ctrl = (event->modifiers() & Qt::ControlModifier);
                bool has_alt = (event->modifiers() & Qt::AltModifier);
                
                bool middle_mouse = (pressed_button_ == Qt::MiddleButton);
                bool emulated_mmb = (pressed_button_ == Qt::LeftButton && has_alt);
                
                if (middle_mouse || emulated_mmb) {
                    if (has_shift && has_ctrl) {
                        // Shift+Ctrl+MMB = Dolly zoom
                        camera_.zoom(delta.y() * 0.1f);
                    } else if (has_ctrl) {
                        // Ctrl+MMB = Zoom
                        camera_.zoom(delta.y() * 0.1f);
                    } else if (has_shift) {
                        // Shift+MMB = Pan
                        camera_.pan(-delta.x(), delta.y());
                    } else {
                        // MMB = Orbit (default)
                        camera_.rotate(-delta.x(), -delta.y());
                    }
                    update(); // Trigger repaint
                }
            }
        }
        last_mouse_pos_ = event->pos();
    }
    
    void wheelEvent(QWheelEvent *event) override {
        // Blender-style wheel navigation with smart mouse/trackpad detection
        QPoint pixelDelta = event->pixelDelta();
        QPoint angleDelta = event->angleDelta();
        Qt::KeyboardModifiers modifiers = event->modifiers();
        
        if (isTrackpadOrSmartMouse(event)) {
            // Handle trackpad/smart mouse with pixel-level precision
            handleGestureWheel(pixelDelta, modifiers);
        } else if (!angleDelta.isNull()) {
            // Handle traditional mouse wheel (discrete steps)
            handleTraditionalWheel(angleDelta, modifiers);
        }
        
        event->accept();
    }
    
    // Handle Qt gesture events for trackpad
    bool event(QEvent *event) override {
        if (event->type() == QEvent::Gesture) {
            return gestureEvent(static_cast<QGestureEvent*>(event));
        }
        return QOpenGLWidget::event(event);
    }
    
    bool gestureEvent(QGestureEvent *event) {
        bool handled = false;
        
        // Handle pinch (zoom) gestures - two finger pinch on trackpad
        if (QGesture *pinch = event->gesture(Qt::PinchGesture)) {
            QPinchGesture *pinchGesture = static_cast<QPinchGesture*>(pinch);
            handlePinchGesture(pinchGesture);
            handled = true;
        }
        
        // Handle pan gestures - two finger scroll on trackpad
        if (QGesture *pan = event->gesture(Qt::PanGesture)) {
            QPanGesture *panGesture = static_cast<QPanGesture*>(pan);
            handlePanGesture(panGesture);
            handled = true;
        }
        
        return handled;
    }
    
    void handlePinchGesture(QPinchGesture *gesture) {
        if (gesture->state() == Qt::GestureStarted) {
            gesture_start_distance_ = camera_.get_distance();
        }
        
        qreal scaleFactor = gesture->scaleFactor();
        qreal totalScaleFactor = gesture->totalScaleFactor();
        
        // Apply zoom based on pinch (like Blender)
        if (totalScaleFactor > 0.0) {
            float newDistance = gesture_start_distance_ / totalScaleFactor;
            camera_.set_distance(newDistance);
            update();
        }
    }
    
    void handlePanGesture(QPanGesture *gesture) {
        QPointF delta = gesture->delta();
        Qt::GestureState state = gesture->state();
        
        switch (state) {
            case Qt::GestureStarted:
                gesture_start_pos_ = gesture->offset();
                break;
                
            case Qt::GestureUpdated: {
                // Two-finger trackpad rotation (like Blender)
                // Use direct delta values - Qt gestures already provide appropriate scaling
                camera_.rotate(-delta.x(), -delta.y());
                update();
                break;
            }
            
            case Qt::GestureFinished:
            case Qt::GestureCanceled:
            case Qt::NoGesture:
                // Gesture complete or no gesture
                break;
        }
    }
};

// Main application window
class VoxeluxMainWindow : public QMainWindow {
    Q_OBJECT

private:
    VoxelOpenGLWidget *opengl_widget_;
    
public:
    VoxeluxMainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Voxelux - Professional Voxel Editor");
        setMinimumSize(1024, 768);
        
        // Create OpenGL widget as central widget
        opengl_widget_ = new VoxelOpenGLWidget(this);
        setCentralWidget(opengl_widget_);
        
        create_menus();
        create_toolbar();
        create_status_bar();
    }

private:
    void create_menus() {
        QMenuBar *menu_bar = menuBar();
        
        // File menu
        QMenu *file_menu = menu_bar->addMenu("&File");
        file_menu->addAction("&New Project", QKeySequence::New, this, &VoxeluxMainWindow::new_project);
        file_menu->addAction("&Open Project", QKeySequence::Open, this, &VoxeluxMainWindow::open_project);
        file_menu->addAction("&Save Project", QKeySequence::Save, this, &VoxeluxMainWindow::save_project);
        file_menu->addSeparator();
        file_menu->addAction("E&xit", QKeySequence::Quit, this, &QWidget::close);
    }
    
    void create_toolbar() {
        QToolBar *main_toolbar = addToolBar("Main");
        main_toolbar->addAction("New", this, &VoxeluxMainWindow::new_project);
        main_toolbar->addAction("Open", this, &VoxeluxMainWindow::open_project);
        main_toolbar->addAction("Save", this, &VoxeluxMainWindow::save_project);
    }
    
    void create_status_bar() {
        statusBar()->showMessage("Ready - Smart Mouse/Trackpad: Swipe to Rotate | MMB: Orbit | Shift+MMB: Pan | Ctrl: Zoom | Blender Controls (OpenGL)", 8000);
    }

private slots:
    void new_project() {
        statusBar()->showMessage("New project created", 2000);
    }
    
    void open_project() {
        statusBar()->showMessage("Open project dialog", 2000);
    }
    
    void save_project() {
        statusBar()->showMessage("Save project dialog", 2000);
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("Voxelux");
    app.setApplicationVersion("0.2.0");
    app.setOrganizationName("Voxelux");
    
    // Check OpenGL support
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4); // Anti-aliasing
    QSurfaceFormat::setDefaultFormat(format);
    
    QOpenGLWidget test_widget;
    test_widget.show();
    if (!test_widget.isValid()) {
        // Try with compatibility profile
        format.setProfile(QSurfaceFormat::CompatibilityProfile);
        QSurfaceFormat::setDefaultFormat(format);
        
        QOpenGLWidget test_widget2;
        test_widget2.show();
        if (!test_widget2.isValid()) {
            QMessageBox::critical(nullptr, "OpenGL Error", 
                "Failed to initialize OpenGL. Please ensure your system supports OpenGL 3.3 or higher.");
            return -1;
        }
        qDebug() << "Using OpenGL Compatibility Profile";
    } else {
        qDebug() << "Using OpenGL Core Profile";
    }
    
    // Create main window
    VoxeluxMainWindow window;
    window.show();
    
    return app.exec();
}

#include "voxelux_opengl.moc"