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
#include "view_animation.h"
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
#include "viewport_nav_widget.h"

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
        // Initialize with 45-degree isometric view: X diagonal bottom-left, Z diagonal bottom-right
        // Y-axis up/down, looking from above
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
    
    void set_rotation(const QQuaternion& rotation) {
        rotation_ = rotation;
        rotation_.normalize();
    }
    
    QQuaternion get_rotation() const { return rotation_; }
    
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
        
        // Create navigation widget
        nav_widget_ = std::make_unique<ViewportNavWidget>(this);
        
        // Initialize professional smooth camera transition system
        camera_transition_ = std::make_unique<CameraTransition>(this);
        camera_transition_->setBaseDuration(100.0f);
    }

private:
    // GPU Resources (Blender-style resource management)
    QOpenGLVertexArrayObject vao_;
    QOpenGLBuffer vertex_buffer_;
    QOpenGLVertexArrayObject vertical_vao_;
    QOpenGLBuffer vertical_vertex_buffer_;
    QOpenGLShaderProgram shader_program_;
    
    // Camera system
    VoxelCamera camera_;
    
    // Navigation state
    bool mouse_pressed_ = false;
    int pressed_button_ = 0;
    QPoint last_mouse_pos_;
    
    // Orthographic view state
    bool is_ortho_side_view_ = false;
    int current_ortho_axis_ = -1; // -1 = none, 0 = X, 1 = Y, 2 = Z
    bool ortho_positive_ = true;
    
    // Grid geometry
    std::vector<float> grid_vertices_;
    std::vector<float> vertical_grid_vertices_;
    int vertex_count_ = 0;
    int vertical_vertex_count_ = 0;
    
    // Viewport widgets
    std::unique_ptr<ViewportNavWidget> nav_widget_;
    
    // Professional smooth camera transition system
    std::unique_ptr<CameraTransition> camera_transition_;
    
    // Gesture tracking
    float gesture_start_distance_ = 0.0f;
    QPointF gesture_start_pos_;
    
    // Scroll direction handling
    bool natural_scroll_direction_ = false;
    
    bool is_side_view() {
        // Only return true if we're in a locked orthographic side view from navigation widget
        return is_ortho_side_view_;
    }
    
    void generate_vertical_grid() {
        vertical_grid_vertices_.clear();
        
        // Generate a simple full-screen quad for the vertical background grid
        // Using normalized coordinates that will fill the viewport
        vertical_grid_vertices_ = {
            // Two triangles forming a full-screen quad
            // First triangle
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // Bottom-left
             1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // Bottom-right
            -1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // Top-left
            
            // Second triangle
            -1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // Top-left
             1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // Bottom-right
             1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f   // Top-right
        };
        
        vertical_vertex_count_ = 6; // Just 6 vertices for 2 triangles
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
    
private slots:
    void onAxisClicked(int axis, bool positive) {
        // Handle navigation widget axis clicks
        float angle_x = 0, angle_y = 0;
        
        // Update orthographic view state
        current_ortho_axis_ = axis;
        ortho_positive_ = positive;
        
        // Only show vertical grid for X and Z axis views (side views)
        is_ortho_side_view_ = (axis == 0 || axis == 2);
        
        switch (axis) {
            case 0: // X axis
                angle_y = positive ? 90.0f : -90.0f;
                angle_x = 0.0f;
                break;
            case 1: // Y axis (top/bottom view - no vertical grid)
                angle_x = positive ? -90.0f : 90.0f;  // +Y looks down, -Y looks up
                angle_y = 0.0f;
                break;
            case 2: // Z axis
                angle_x = 0.0f;
                angle_y = positive ? 0.0f : 180.0f;
                break;
        }
        
        // Start professional smooth camera transition
        QQuaternion target_rotation = QQuaternion::fromEulerAngles(angle_x, angle_y, 0.0f);
        camera_transition_->startRotationTransition(
            camera_.get_rotation(),
            target_rotation,
            [this](const QQuaternion& rotation) {
                camera_.set_rotation(rotation);
                update(); // Trigger viewport refresh
            }
        );
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
            qDebug() << "Trackpad wheel rotation - delta:" << adjustedDelta.x() << adjustedDelta.y();
            camera_.rotate(-adjustedDelta.x(), -adjustedDelta.y());
            
            // Clear orthographic view state when rotating via trackpad
            if (adjustedDelta.x() != 0 || adjustedDelta.y() != 0) {
                qDebug() << "Cleared ortho view due to trackpad wheel rotation - was ortho:" << is_ortho_side_view_;
                is_ortho_side_view_ = false;
                current_ortho_axis_ = -1;
            }
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
        
        // Initialize navigation widget
        if (nav_widget_) {
            nav_widget_->initialize();
            nav_widget_->setPosition(QPointF(30.0f, 30.0f)); // Position in bottom-right with margin
            nav_widget_->setSize(80.0f); // Size in pixels
            nav_widget_->setAxisClickCallback([this](int axis, bool positive) {
                onAxisClicked(axis, positive);
            });
            
            // Configure text offsets for fine-tuning label positions
            // Positive values move right/down, negative values move left/up
            // nav_widget_->setTextOffsetY(0.0f, -1.0f);      // Move Y label up 1 pixel
            // nav_widget_->setTextOffsetZ(0.0f, 1.0f);       // Move Z label down 1 pixel
            // nav_widget_->setTextOffsetNegX(0.5f, 0.0f);    // Move -X label right 0.5 pixels
            nav_widget_->setTextOffsetY(0.0f, 1.0f);
            nav_widget_->setTextOffsetX(0.0f, 1.0f);
            nav_widget_->setTextOffsetZ(0.0f, 1.0f);
        }
        
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
                // Grid vertices come in as XZ plane (x, 0, z), transform based on plane_axes
                vec3 vert_pos;
                if (plane_axes.x > 0.5 && plane_axes.y > 0.5) {
                    // XY plane (front/back view) - map XZ to XY
                    vert_pos = vec3(position.x, position.z, 0.0);
                } else if (plane_axes.x > 0.5 && plane_axes.z > 0.5) {
                    // XZ plane (top/bottom view - horizontal grid) - use as-is
                    vert_pos = position;
                } else if (plane_axes.y > 0.5 && plane_axes.z > 0.5) {
                    // YZ plane (left/right view) - map XZ to YZ
                    vert_pos = vec3(0.0, position.x, position.z);
                } else {
                    // Default XZ plane
                    vert_pos = position;
                }
                
                // Scale grid to world size
                vec3 real_pos = vert_pos * grid_size;
                
                world_pos = real_pos;
                local_pos = position;
                
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
            uniform mat4 projection;
            
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
                
                // Grid calculations on active plane based on plane_axes uniform (like Blender)
                vec2 grid_pos, grid_fwidth;
                if (plane_axes.x > 0.5 && plane_axes.y > 0.5) {
                    // XY plane (front/back view)
                    grid_pos = P.xy;
                    grid_fwidth = fwidthPos.xy;
                } else if (plane_axes.x > 0.5 && plane_axes.z > 0.5) {
                    // XZ plane (top/bottom view - horizontal grid)
                    grid_pos = P.xz;
                    grid_fwidth = fwidthPos.xz;
                } else if (plane_axes.y > 0.5 && plane_axes.z > 0.5) {
                    // YZ plane (left/right view)
                    grid_pos = P.yz;
                    grid_fwidth = fwidthPos.yz;
                } else {
                    // Default XZ plane
                    grid_pos = P.xz;
                    grid_fwidth = fwidthPos.xz;
                }
                
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
                
                // Calculate axis lines based on active plane
                vec3 axes_dist = vec3(0.0);
                vec3 axes_fwidth = vec3(0.0);
                
                if (plane_axes.x > 0.5 && plane_axes.y > 0.5) {
                    // XY plane (front/back view)
                    // X-axis (red) at Y=0
                    axes_dist.x = P.y;
                    axes_fwidth.x = fwidthPos.y;
                    
                    // Y-axis (green, using Z uniform) at X=0
                    axes_dist.z = P.x;
                    axes_fwidth.z = fwidthPos.x;
                } else if (plane_axes.x > 0.5 && plane_axes.z > 0.5) {
                    // XZ plane (top/bottom view - horizontal grid)
                    // X-axis (red) at Z=0
                    axes_dist.x = P.z;
                    axes_fwidth.x = fwidthPos.z;
                    
                    // Z-axis (blue) at X=0
                    axes_dist.z = P.x;
                    axes_fwidth.z = fwidthPos.x;
                } else if (plane_axes.y > 0.5 && plane_axes.z > 0.5) {
                    // YZ plane (left/right view)
                    // Y-axis (green, using X uniform) at Z=0
                    axes_dist.x = P.z;
                    axes_fwidth.x = fwidthPos.z;
                    
                    // Z-axis (blue) at Y=0
                    axes_dist.z = P.y;
                    axes_fwidth.z = fwidthPos.y;
                } else {
                    // Default XZ plane
                    axes_dist.x = P.z;
                    axes_fwidth.x = fwidthPos.z;
                    axes_dist.z = P.x;
                    axes_fwidth.z = fwidthPos.x;
                }
                
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
        
        // Set up vertical grid VAO and VBO
        generate_vertical_grid();
        
        vertical_vao_.create();
        vertical_vao_.bind();
        
        vertical_vertex_buffer_.create();
        vertical_vertex_buffer_.bind();
        vertical_vertex_buffer_.allocate(vertical_grid_vertices_.data(), vertical_grid_vertices_.size() * sizeof(float));
        
        // Set up vertical grid vertex attributes (same layout as horizontal grid)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        vertical_vao_.release();
        vertical_vertex_buffer_.release();
        
        qDebug() << "OpenGL viewport initialized with" << vertex_count_ << "horizontal and" << vertical_vertex_count_ << "vertical vertices";
    }
    
    void paintGL() override {
        static int frame_count = 0;
        if (frame_count++ % 60 == 0) {
            float grid_size = qMax(2000.0f, camera_.get_distance() * 6.0f);
            QMatrix4x4 view = camera_.get_view_matrix();
            QVector3D view_pos = view.inverted().column(3).toVector3D();
            
            // Debug side view detection
            QQuaternion rotation = camera_.get_rotation();
            QVector3D forward = rotation.rotatedVector(QVector3D(0, 0, -1));
            bool side_view = is_side_view();
            
            qDebug() << "Frame" << frame_count 
                     << "Camera distance:" << camera_.get_distance()
                     << "Grid size:" << grid_size
                     << "Forward:" << forward.x() << forward.y() << forward.z()
                     << "Side view:" << side_view;
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
        
        // Draw vertical grid in side view (using same geometry as horizontal grid)
        if (is_side_view()) {
            static int debug_count = 0;
            if (debug_count++ % 60 == 0) {
                qDebug() << "Drawing vertical grid - ortho axis:" << current_ortho_axis_ << "is_ortho:" << is_ortho_side_view_;
            }
            // Determine plane based on which navigation sphere was clicked
            if (current_ortho_axis_ == 0) {
                // X axis view - YZ plane (left/right view)
                shader_program_.setUniformValue("plane_axes", QVector3D(0.0f, 1.0f, 1.0f)); // Y and Z active
                shader_program_.setUniformValue("grid_axis_x_color", QVector3D(0.39f, 0.78f, 0.39f)); // Green Y-axis (using X uniform)
                shader_program_.setUniformValue("grid_axis_z_color", QVector3D(0.22f, 0.53f, 0.86f)); // Blue Z-axis
            } else if (current_ortho_axis_ == 2) {
                // Z axis view - XY plane (front/back view)
                shader_program_.setUniformValue("plane_axes", QVector3D(1.0f, 1.0f, 0.0f)); // X and Y active
                shader_program_.setUniformValue("grid_axis_x_color", QVector3D(0.96f, 0.26f, 0.31f)); // Red X-axis
                shader_program_.setUniformValue("grid_axis_z_color", QVector3D(0.39f, 0.78f, 0.39f)); // Green Y-axis (using Z uniform)
            }
            
            // Draw vertical grid using the same grid geometry
            vao_.bind();
            glDrawArrays(GL_TRIANGLES, 0, vertex_count_);
            vao_.release();
        }
        
        // Always draw horizontal grid (XZ plane)
        shader_program_.setUniformValue("plane_axes", QVector3D(1.0f, 0.0f, 1.0f)); // X and Z active (XZ plane)
        shader_program_.setUniformValue("grid_axis_x_color", QVector3D(0.96f, 0.26f, 0.31f)); // Red X-axis
        shader_program_.setUniformValue("grid_axis_z_color", QVector3D(0.22f, 0.53f, 0.86f)); // Blue Z-axis
        
        vao_.bind();
        glDrawArrays(GL_TRIANGLES, 0, vertex_count_);
        vao_.release();
        
        shader_program_.release();
        
        // Render navigation widget
        if (nav_widget_ && nav_widget_->isVisible()) {
            nav_widget_->render(view, projection);
        }
        
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
        // Try navigation widget first
        if (nav_widget_ && nav_widget_->handleMousePress(event, camera_.get_view_matrix(), camera_.get_projection_matrix(float(width()) / height()))) {
            update();
            return;
        }
        
        mouse_pressed_ = true;
        pressed_button_ = event->button();
        last_mouse_pos_ = event->pos();
    }
    
    void mouseReleaseEvent(QMouseEvent *event) override {
        // Try navigation widget first
        if (nav_widget_ && nav_widget_->handleMouseRelease(event, camera_.get_view_matrix(), camera_.get_projection_matrix(float(width()) / height()))) {
            update();
            return;
        }
        
        mouse_pressed_ = false;
        pressed_button_ = 0;
    }
    
    void mouseMoveEvent(QMouseEvent *event) override {
        // Try navigation widget first (for hover effects)
        if (nav_widget_ && nav_widget_->handleMouseMove(event, camera_.get_view_matrix(), camera_.get_projection_matrix(float(width()) / height()))) {
            update();
            if (!mouse_pressed_) return;
        }
        
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
                        // Zooming is OK - keep ortho view
                    } else if (has_ctrl) {
                        // Ctrl+MMB = Zoom
                        camera_.zoom(delta.y() * 0.1f);
                        // Zooming is OK - keep ortho view
                    } else if (has_shift) {
                        // Shift+MMB = Pan
                        camera_.pan(-delta.x(), delta.y());
                        // Panning is OK - keep ortho view
                    } else {
                        // MMB = Orbit (default) - ANY rotation clears ortho view
                        qDebug() << "Rotating camera - delta:" << delta.x() << delta.y();
                        camera_.rotate(-delta.x(), -delta.y());
                        // Clear orthographic view state when rotating in any direction
                        if (delta.x() != 0 || delta.y() != 0) {
                            qDebug() << "Cleared ortho view due to rotation - was ortho:" << is_ortho_side_view_;
                            is_ortho_side_view_ = false;
                            current_ortho_axis_ = -1;
                        }
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
                qDebug() << "Trackpad rotation - delta:" << delta.x() << delta.y();
                camera_.rotate(-delta.x(), -delta.y());
                
                // Clear orthographic view state when rotating via trackpad
                if (delta.x() != 0 || delta.y() != 0) {
                    qDebug() << "Cleared ortho view due to trackpad rotation - was ortho:" << is_ortho_side_view_;
                    is_ortho_side_view_ = false;
                    current_ortho_axis_ = -1;
                }
                
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