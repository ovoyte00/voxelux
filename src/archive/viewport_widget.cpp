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

#include "viewport_widget.h"
#include <QOpenGLContext>

ViewportWidget::ViewportWidget(QOpenGLWidget* parent)
    : parent_widget_(parent)
{
}

ViewportWidget::~ViewportWidget()
{
    cleanup();
}

void ViewportWidget::cleanup()
{
    // Base cleanup - derived classes should override and call this
}

bool ViewportWidget::handleMousePress(QMouseEvent* event, const QMatrix4x4& view, const QMatrix4x4& projection)
{
    if (!visible_) return false;
    
    int hit = hitTest(event->pos(), view, projection);
    if (hit >= 0) {
        highlighted_ = true;
        highlighted_part_ = hit;
        dragging_ = true;
        // Interaction started callback could go here
        return true;
    }
    return false;
}

bool ViewportWidget::handleMouseMove(QMouseEvent* event, const QMatrix4x4& view, const QMatrix4x4& projection)
{
    if (!visible_) return false;
    
    if (dragging_) {
        // Interaction updated callback could go here
        return true;
    } else {
        int hit = hitTest(event->pos(), view, projection);
        bool was_highlighted = highlighted_;
        int old_part = highlighted_part_;
        
        highlighted_ = (hit >= 0);
        highlighted_part_ = hit;
        
        if (highlighted_ != was_highlighted || highlighted_part_ != old_part) {
            if (parent_widget_) parent_widget_->update();
        }
        
        return highlighted_;
    }
}

bool ViewportWidget::handleMouseRelease(QMouseEvent* event, const QMatrix4x4& view, const QMatrix4x4& projection)
{
    if (!visible_) return false;
    
    if (dragging_) {
        dragging_ = false;
        // Interaction ended callback could go here
        
        int hit = hitTest(event->pos(), view, projection);
        if (hit >= 0 && hit == highlighted_part_) {
            if (click_callback_) {
                click_callback_(hit);
            }
        }
        return true;
    }
    return false;
}

bool ViewportWidget::handleWheel(QWheelEvent* event)
{
    return false; // Most widgets don't handle wheel
}

QPointF ViewportWidget::projectToScreen(const QVector3D& worldPos, const QMatrix4x4& view, const QMatrix4x4& projection)
{
    QVector4D pos(worldPos, 1.0f);
    pos = projection * view * pos;
    
    if (pos.w() != 0.0f) {
        pos /= pos.w();
    }
    
    // Convert from NDC [-1,1] to screen coordinates
    float x = (pos.x() + 1.0f) * 0.5f * parent_widget_->width();
    float y = (1.0f - pos.y()) * 0.5f * parent_widget_->height(); // Flip Y
    
    return QPointF(x, y);
}

void ViewportWidget::unprojectScreenToRay(const QPoint& screenPos, const QMatrix4x4& view, const QMatrix4x4& projection,
                                          QVector3D& rayOrigin, QVector3D& rayDirection)
{
    // Convert screen coordinates to NDC
    float x = (2.0f * screenPos.x()) / parent_widget_->width() - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y()) / parent_widget_->height();
    
    // Create near and far points in NDC
    QVector4D nearPoint(x, y, -1.0f, 1.0f);
    QVector4D farPoint(x, y, 1.0f, 1.0f);
    
    // Unproject
    QMatrix4x4 invVP = (projection * view).inverted();
    nearPoint = invVP * nearPoint;
    farPoint = invVP * farPoint;
    
    // Perspective divide
    if (nearPoint.w() != 0.0f) nearPoint /= nearPoint.w();
    if (farPoint.w() != 0.0f) farPoint /= farPoint.w();
    
    // Create ray
    rayOrigin = nearPoint.toVector3D();
    rayDirection = (farPoint.toVector3D() - rayOrigin).normalized();
}

bool ViewportWidget::loadShaders(QOpenGLShaderProgram& program, const QString& vertexPath, const QString& fragmentPath)
{
    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, vertexPath)) {
        qWarning() << "Failed to compile vertex shader:" << program.log();
        return false;
    }
    
    if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, fragmentPath)) {
        qWarning() << "Failed to compile fragment shader:" << program.log();
        return false;
    }
    
    if (!program.link()) {
        qWarning() << "Failed to link shader program:" << program.log();
        return false;
    }
    
    return true;
}