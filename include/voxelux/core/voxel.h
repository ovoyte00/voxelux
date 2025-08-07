/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Professional voxel data structures and types.
 * Independent implementation of voxel representation system.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace voxelux::core {

struct Color {
    uint8_t r, g, b, a;
    
    Color() : r(255), g(255), b(255), a(255) {}
    Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}
    
    bool operator==(const Color& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
};

struct Material {
    std::string name;
    Color color;
    std::string texture_path;
    float metallic = 0.0f;
    float roughness = 0.5f;
    float emission = 0.0f;
    
    Material() = default;
    Material(const std::string& material_name, const Color& material_color)
        : name(material_name), color(material_color) {}
};

class Voxel {
public:
    Voxel() : material_id_(0), active_(false) {}
    explicit Voxel(uint32_t material_id) : material_id_(material_id), active_(true) {}
    
    bool is_active() const { return active_; }
    void set_active(bool active) { active_ = active; }
    
    uint32_t material_id() const { return material_id_; }
    void set_material_id(uint32_t id) { material_id_ = id; active_ = (id != 0); }
    
    bool operator==(const Voxel& other) const {
        return material_id_ == other.material_id_ && active_ == other.active_;
    }
    
private:
    uint32_t material_id_;
    bool active_;
};

}