/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Professional voxel data structure and management system.
 * Independent implementation of 3D voxel grid with efficient storage.
 */

#pragma once

#include "voxel.h"
#include "vector3.h"
#include <vector>
#include <unordered_map>
#include <memory>

namespace voxelux::core {

class VoxelGrid {
public:
    VoxelGrid(const Vector3i& dimensions);
    VoxelGrid(int width, int height, int depth);
    
    const Vector3i& dimensions() const { return dimensions_; }
    
    bool is_valid_position(const Vector3i& pos) const;
    size_t get_index(const Vector3i& pos) const;
    Vector3i get_position(size_t index) const;
    
    const Voxel& get_voxel(const Vector3i& pos) const;
    const Voxel& get_voxel(int x, int y, int z) const;
    
    void set_voxel(const Vector3i& pos, const Voxel& voxel);
    void set_voxel(int x, int y, int z, const Voxel& voxel);
    
    void clear();
    void fill(const Voxel& voxel);
    
    bool is_empty() const;
    size_t active_voxel_count() const;
    
    Vector3i min_bounds() const;
    Vector3i max_bounds() const;
    
    class Iterator {
    public:
        Iterator(const VoxelGrid* grid, size_t index) : grid_(grid), index_(index) {}
        
        bool operator!=(const Iterator& other) const { return index_ != other.index_; }
        Iterator& operator++() { ++index_; return *this; }
        
        struct VoxelData {
            Vector3i position;
            const Voxel& voxel;
        };
        
        VoxelData operator*() const {
            return {grid_->get_position(index_), grid_->voxels_[index_]};
        }
        
    private:
        const VoxelGrid* grid_;
        size_t index_;
    };
    
    Iterator begin() const { return Iterator(this, 0); }
    Iterator end() const { return Iterator(this, voxels_.size()); }
    
private:
    Vector3i dimensions_;
    std::vector<Voxel> voxels_;
    static const Voxel empty_voxel_;
};

class MaterialRegistry {
public:
    uint32_t add_material(const Material& material);
    const Material& get_material(uint32_t id) const;
    bool has_material(uint32_t id) const;
    void remove_material(uint32_t id);
    
    size_t material_count() const { return materials_.size(); }
    
    class Iterator {
    public:
        using MapIterator = std::unordered_map<uint32_t, Material>::const_iterator;
        
        Iterator(MapIterator it) : it_(it) {}
        bool operator!=(const Iterator& other) const { return it_ != other.it_; }
        Iterator& operator++() { ++it_; return *this; }
        
        const std::pair<const uint32_t, Material>& operator*() const { return *it_; }
        
    private:
        MapIterator it_;
    };
    
    Iterator begin() const { return Iterator(materials_.begin()); }
    Iterator end() const { return Iterator(materials_.end()); }
    
private:
    std::unordered_map<uint32_t, Material> materials_;
    uint32_t next_id_ = 1;
    static const Material default_material_;
};

}