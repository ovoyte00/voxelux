/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Professional 3D viewport widget system.
 * Independent implementation using Qt OpenGL framework.
 */

#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include <QVector3D>
#include <QMouseEvent>
#include <functional>

/**
 * Base class for all viewport overlay widgets (navigation, transform gizmos, etc.)
 * Provides common functionality for interactive 3D widgets rendered over the viewport.
 */
class ViewportWidget : protected QOpenGLFunctions_3_3_Core
{
public:
    ViewportWidget(QOpenGLWidget* parent = nullptr);
    virtual ~ViewportWidget();

    // Lifecycle methods
    virtual void initialize() = 0;
    virtual void cleanup();
    
    // Rendering
    virtual void render(const QMatrix4x4& view, const QMatrix4x4& projection) = 0;
    
    // Interaction
    virtual bool handleMousePress(QMouseEvent* event, const QMatrix4x4& view, const QMatrix4x4& projection);
    virtual bool handleMouseMove(QMouseEvent* event, const QMatrix4x4& view, const QMatrix4x4& projection);
    virtual bool handleMouseRelease(QMouseEvent* event, const QMatrix4x4& view, const QMatrix4x4& projection);
    virtual bool handleWheel(QWheelEvent* event);
    
    // Visibility
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }
    
    // Position and size
    void setPosition(const QPointF& pos) { position_ = pos; }
    QPointF position() const { return position_; }
    
    void setSize(float size) { size_ = size; }
    float size() const { return size_; }
    
    // Highlighting
    bool isHighlighted() const { return highlighted_; }
    int highlightedPart() const { return highlighted_part_; }

    // Callback function type for widget interaction
    typedef std::function<void(int part)> WidgetClickCallback;
    
    // Set callback for widget clicks
    void setClickCallback(WidgetClickCallback callback) { click_callback_ = callback; }

protected:
    // Hit testing
    virtual int hitTest(const QPoint& pos, const QMatrix4x4& view, const QMatrix4x4& projection) { return -1; }
    
    // Helper to project 3D point to screen
    QPointF projectToScreen(const QVector3D& worldPos, const QMatrix4x4& view, const QMatrix4x4& projection);
    
    // Helper to unproject screen point to ray
    void unprojectScreenToRay(const QPoint& screenPos, const QMatrix4x4& view, const QMatrix4x4& projection,
                              QVector3D& rayOrigin, QVector3D& rayDirection);

    QOpenGLWidget* parent_widget_;
    
    // State
    bool visible_ = true;
    bool highlighted_ = false;
    int highlighted_part_ = -1;
    bool dragging_ = false;
    
    // Position and size
    QPointF position_;  // Screen position (for 2D widgets) or anchor point
    float size_ = 80.0f;  // Widget size in pixels
    
    // Common shader management
    bool loadShaders(QOpenGLShaderProgram& program, const QString& vertexPath, const QString& fragmentPath);
    
    // Callback for widget clicks
    WidgetClickCallback click_callback_;
};