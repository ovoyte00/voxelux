#include "voxelux/core/voxel_grid.h"
#include <algorithm>
#include <stdexcept>

namespace voxelux::core {

const Voxel VoxelGrid::empty_voxel_;
const Material MaterialRegistry::default_material_{"Default", Color(128, 128, 128)};

VoxelGrid::VoxelGrid(const Vector3i& dimensions) 
    : dimensions_(dimensions), voxels_(dimensions.x * dimensions.y * dimensions.z) {
    if (dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0) {
        throw std::invalid_argument("Grid dimensions must be positive");
    }
}

VoxelGrid::VoxelGrid(int width, int height, int depth) 
    : VoxelGrid(Vector3i(width, height, depth)) {}

bool VoxelGrid::is_valid_position(const Vector3i& pos) const {
    return pos.x >= 0 && pos.x < dimensions_.x &&
           pos.y >= 0 && pos.y < dimensions_.y &&
           pos.z >= 0 && pos.z < dimensions_.z;
}

size_t VoxelGrid::get_index(const Vector3i& pos) const {
    if (!is_valid_position(pos)) {
        throw std::out_of_range("Position out of grid bounds");
    }
    return pos.x + pos.y * dimensions_.x + pos.z * dimensions_.x * dimensions_.y;
}

Vector3i VoxelGrid::get_position(size_t index) const {
    int z = index / (dimensions_.x * dimensions_.y);
    int y = (index % (dimensions_.x * dimensions_.y)) / dimensions_.x;
    int x = index % dimensions_.x;
    return Vector3i(x, y, z);
}

const Voxel& VoxelGrid::get_voxel(const Vector3i& pos) const {
    if (!is_valid_position(pos)) {
        return empty_voxel_;
    }
    return voxels_[get_index(pos)];
}

const Voxel& VoxelGrid::get_voxel(int x, int y, int z) const {
    return get_voxel(Vector3i(x, y, z));
}

void VoxelGrid::set_voxel(const Vector3i& pos, const Voxel& voxel) {
    if (is_valid_position(pos)) {
        voxels_[get_index(pos)] = voxel;
    }
}

void VoxelGrid::set_voxel(int x, int y, int z, const Voxel& voxel) {
    set_voxel(Vector3i(x, y, z), voxel);
}

void VoxelGrid::clear() {
    std::fill(voxels_.begin(), voxels_.end(), Voxel());
}

void VoxelGrid::fill(const Voxel& voxel) {
    std::fill(voxels_.begin(), voxels_.end(), voxel);
}

bool VoxelGrid::is_empty() const {
    return std::all_of(voxels_.begin(), voxels_.end(), 
                      [](const Voxel& v) { return !v.is_active(); });
}

size_t VoxelGrid::active_voxel_count() const {
    return std::count_if(voxels_.begin(), voxels_.end(), 
                        [](const Voxel& v) { return v.is_active(); });
}

Vector3i VoxelGrid::min_bounds() const {
    Vector3i min_pos = dimensions_;
    for (const auto& voxel_data : *this) {
        if (voxel_data.voxel.is_active()) {
            min_pos.x = std::min(min_pos.x, voxel_data.position.x);
            min_pos.y = std::min(min_pos.y, voxel_data.position.y);
            min_pos.z = std::min(min_pos.z, voxel_data.position.z);
        }
    }
    return min_pos;
}

Vector3i VoxelGrid::max_bounds() const {
    Vector3i max_pos = Vector3i(-1, -1, -1);
    for (const auto& voxel_data : *this) {
        if (voxel_data.voxel.is_active()) {
            max_pos.x = std::max(max_pos.x, voxel_data.position.x);
            max_pos.y = std::max(max_pos.y, voxel_data.position.y);
            max_pos.z = std::max(max_pos.z, voxel_data.position.z);
        }
    }
    return max_pos;
}

uint32_t MaterialRegistry::add_material(const Material& material) {
    uint32_t id = next_id_++;
    materials_[id] = material;
    return id;
}

const Material& MaterialRegistry::get_material(uint32_t id) const {
    auto it = materials_.find(id);
    if (it != materials_.end()) {
        return it->second;
    }
    return default_material_;
}

bool MaterialRegistry::has_material(uint32_t id) const {
    return materials_.find(id) != materials_.end();
}

void MaterialRegistry::remove_material(uint32_t id) {
    materials_.erase(id);
}

}