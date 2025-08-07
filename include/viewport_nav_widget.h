#pragma once

#include "viewport_widget.h"
#include "text_renderer.h"
#include <array>
#include <vector>
#include <map>

/**
 * Navigation widget for 3D viewport - professional axis orientation control.
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
    
    // Override mouse handling for axis click callbacks and hover detection
    bool handleMouseMove(QMouseEvent* event, const QMatrix4x4& view, const QMatrix4x4& projection) override;
    bool handleMouseRelease(QMouseEvent* event, const QMatrix4x4& view, const QMatrix4x4& projection) override;

    // Widget-specific settings
    void setAxisColors(const QVector3D& xColor, const QVector3D& yColor, const QVector3D& zColor);
    void setBackgroundAlpha(float alpha) { background_alpha_ = alpha; }
    void setShowAxisLabels(bool show) { show_labels_ = show; }
    
    // Text offset settings - individual control for each axis label
    void setTextOffsetX(float offsetX, float offsetY) { text_offsets_["X"] = {offsetX, offsetY}; }
    void setTextOffsetY(float offsetX, float offsetY) { text_offsets_["Y"] = {offsetX, offsetY}; }
    void setTextOffsetZ(float offsetX, float offsetY) { text_offsets_["Z"] = {offsetX, offsetY}; }
    void setTextOffsetNegX(float offsetX, float offsetY) { text_offsets_["-X"] = {offsetX, offsetY}; }
    void setTextOffsetNegY(float offsetX, float offsetY) { text_offsets_["-Y"] = {offsetX, offsetY}; }
    void setTextOffsetNegZ(float offsetX, float offsetY) { text_offsets_["-Z"] = {offsetX, offsetY}; }
    void clearTextOffsets() { text_offsets_.clear(); }

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
    
    // Helper methods for geometry generation (professional 3D approach)
    void generateCylindricalLine(float x1, float y1, float z1, float x2, float y2, float z2,
                                const float color[3], float alpha, float radius);
    void generateSphere(float cx, float cy, float cz, float radius,
                       const float color[3], float alpha, int lat_segments, int lon_segments);
    void generateSphereOutline(float cx, float cy, float cz, float radius,
                              const float color[3], float alpha, int lat_segments, int lon_segments, float outline_width);
    void generateSimpleLine(float x1, float y1, float z1, float x2, float y2, float z2,
                           const float color[3], float alpha, float line_width);
    void renderScreenSpaceBillboards(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection);
    void renderScreenSpaceElements(const QMatrix4x4& view, const QMatrix4x4& projection);
    void generateScreenSpaceCircle(float cx, float cy, float radius, float r, float g, float b, float alpha, std::vector<float>& vertices);
    void generateScreenSpaceHollowCircle(float cx, float cy, float radius, 
                                         float bg_r, float bg_g, float bg_b, float bg_alpha,
                                         float axis_r, float axis_g, float axis_b, float axis_alpha,
                                         float ring_r, float ring_g, float ring_b, float ring_alpha, 
                                         std::vector<float>& vertices);
    void generateScreenSpaceLine(float x1, float y1, float x2, float y2, float width, float r, float g, float b, float alpha, std::vector<float>& vertices);
    void renderTextForSphere(int billboard_index, int sphere_index, float screen_x, float screen_y, const QMatrix4x4& view, const QMatrix4x4& projection);
    void renderHoverBackground(float x, float y, const QMatrix4x4& projection);
    
    // Sphere data for axis indicators
    struct AxisSphere {
        int axis;      // 0=X, 1=Y, 2=Z
        bool positive; // true for +X/+Y/+Z, false for -X/-Y/-Z
        float depth;   // Depth value for sorting
        QVector3D position;
        QVector3D color;
    };
    
    // Billboard data for screen-space rendering
    struct BillboardData {
        QVector3D position; // 3D world position
        float radius;       // Screen-space radius
        QVector3D color;    // RGB color
        bool positive;      // true for solid, false for hollow
        float depth;        // Depth for sorting
        int sphere_index;   // Original sphere index (0-5) for correct text mapping
    };
    
    // Line data for screen-space rendering
    struct LineData {
        QVector3D start;    // 3D world start position
        QVector3D end;      // 3D world end position  
        QVector3D color;    // RGB color
        float alpha;        // Line alpha
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
    std::vector<BillboardData> billboard_data_; // Billboard data for screen-space rendering
    std::vector<LineData> line_data_; // Line data for screen-space rendering
    
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
    
    // Hover state tracking
    int hovered_sphere_ = -1; // -1 = none, 0-5 = sphere index
    
    // Text rendering system
    TextRenderer text_renderer_;
    
    // Text offset storage - maps axis label to {offsetX, offsetY}
    std::map<QString, std::pair<float, float>> text_offsets_;
};