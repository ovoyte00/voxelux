/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Voxel Canvas UI - Core implementation
 */

#include "canvas_ui/canvas_core.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace voxel_canvas {

// Rect2D implementation
bool Rect2D::contains(const Point2D& point) const {
    return point.x >= x && point.x < x + width &&
           point.y >= y && point.y < y + height;
}

bool Rect2D::intersects(const Rect2D& other) const {
    return !(other.x >= x + width || other.x + other.width <= x ||
             other.y >= y + height || other.y + other.height <= y);
}

Rect2D Rect2D::intersection(const Rect2D& other) const {
    if (!intersects(other)) {
        return Rect2D(0, 0, 0, 0);
    }
    
    float left = std::max(x, other.x);
    float top = std::max(y, other.y);
    float right = std::min(x + width, other.x + other.width);
    float bottom = std::min(y + height, other.y + other.height);
    
    return Rect2D(left, top, right - left, bottom - top);
}

// ColorRGBA implementation
ColorRGBA::ColorRGBA(const std::string& hex) : r(0), g(0), b(0), a(1) {
    if (hex.empty() || hex[0] != '#') return;
    
    std::string hex_clean = hex.substr(1);
    if (hex_clean.length() == 6) {
        // #RRGGBB format
        std::stringstream ss;
        ss << std::hex << hex_clean;
        uint32_t rgb;
        ss >> rgb;
        
        r = ((rgb >> 16) & 0xFF) / 255.0f;
        g = ((rgb >> 8) & 0xFF) / 255.0f;
        b = (rgb & 0xFF) / 255.0f;
    } else if (hex_clean.length() == 8) {
        // #RRGGBBAA format
        std::stringstream ss;
        ss << std::hex << hex_clean;
        uint32_t rgba;
        ss >> rgba;
        
        r = ((rgba >> 24) & 0xFF) / 255.0f;
        g = ((rgba >> 16) & 0xFF) / 255.0f;
        b = ((rgba >> 8) & 0xFF) / 255.0f;
        a = (rgba & 0xFF) / 255.0f;
    }
}

// InputEvent implementation  
bool InputEvent::is_mouse_event() const {
    return type == EventType::MOUSE_PRESS ||
           type == EventType::MOUSE_RELEASE ||
           type == EventType::MOUSE_MOVE ||
           type == EventType::MOUSE_WHEEL;
}

bool InputEvent::is_keyboard_event() const {
    return type == EventType::KEY_PRESS ||
           type == EventType::KEY_RELEASE;
}

} // namespace voxel_canvas