#pragma once

#include "viewport_widget.h"
#include <array>
#include <vector>

/**
 * Navigation widget for 3D viewport - similar to Blender's navigation gizmo.
 * Provides clickable axis indicators for quick view orientation changes.
 */
class ViewportNavWidget : public ViewportWidget
{
public:
    ViewportNavWidget(QOpenGLWidget* parent = nullptr);
    ~ViewportNavWidget() override;

    // ViewportWidget interface
    void initialize() override;
    void cleanup() override;
    void render(const QMatrix4x4& view, const QMatrix4x4& projection) override;
    
    // Override mouse handling for axis click callbacks
    bool handleMouseRelease(QMouseEvent* event, const QMatrix4x4& view, const QMatrix4x4& projection) override;

    // Widget-specific settings
    void setAxisColors(const QVector3D& xColor, const QVector3D& yColor, const QVector3D& zColor);
    void setBackgroundAlpha(float alpha) { background_alpha_ = alpha; }
    void setShowAxisLabels(bool show) { show_labels_ = show; }

    // Callback function type for axis clicks
    typedef std::function<void(int axis, bool positive)> AxisClickCallback;
    
    // Set callback for axis clicks  
    void setAxisClickCallback(AxisClickCallback callback) { axis_click_callback_ = callback; }

protected:
    int hitTest(const QPoint& pos, const QMatrix4x4& view, const QMatrix4x4& projection) override;

private:
    // Initialize geometry
    void createWidgetGeometry();
    void createSphereGeometry();
    void regenerateSortedWidget(const QMatrix4x4& camera_view);
    
    // Helper methods for 3D geometry generation (matching working implementation)
    void generateCylindricalLine(float x1, float y1, float z1, float x2, float y2, float z2,
                                const float color[3], float alpha, float radius);
    void generateSphere(float cx, float cy, float cz, float radius,
                       const float color[3], float alpha, int lat_segments, int lon_segments);
    void generateSphereOutline(float cx, float cy, float cz, float radius,
                              const float color[3], float alpha, int lat_segments, int lon_segments, float outline_width);
    
    // Sphere data for axis indicators
    struct AxisSphere {
        int axis;      // 0=X, 1=Y, 2=Z
        bool positive; // true for +X/+Y/+Z, false for -X/-Y/-Z
        float depth;   // Depth value for sorting
        QVector3D position;
        QVector3D color;
    };
    
    // Shader programs
    QOpenGLShaderProgram widget_shader_program_;
    
    // Vertex data
    QOpenGLVertexArrayObject widget_vao_;
    QOpenGLBuffer widget_vbo_;
    std::vector<float> widget_vertices_;
    int widget_vertex_count_ = 0;
    
    QOpenGLVertexArrayObject sphere_vao_;
    QOpenGLBuffer sphere_vbo_;
    std::vector<float> sphere_vertices_;
    int sphere_segments_ = 16;
    
    // Sphere data
    std::array<AxisSphere, 6> sphere_data_;
    std::vector<float> sphere_template_; // Template sphere vertices
    
    // Appearance settings
    QVector3D axis_colors_[3] = {
        QVector3D(1.0f, 0.0f, 0.0f), // X = Red
        QVector3D(0.0f, 1.0f, 0.0f), // Y = Green  
        QVector3D(0.0f, 0.0f, 1.0f)  // Z = Blue
    };
    float background_alpha_ = 0.3f;
    bool show_labels_ = true;
    
    // Widget geometry parameters
    float axis_length_ = 0.75f;
    float sphere_radius_ = 0.2f;
    float axis_thickness_ = 0.02f;
    
    // Callback for axis clicks
    AxisClickCallback axis_click_callback_;
};