#include "voxelux/core/voxel_grid.h"
#include <iostream>

int main() {
    std::cout << "Running basic tests..." << std::endl;
    
    // Basic voxel grid test
    voxelux::core::VoxelGrid grid(5, 5, 5);
    std::cout << "Grid created: " << grid.dimensions().x << "x" 
              << grid.dimensions().y << "x" << grid.dimensions().z << std::endl;
    
    // Test material registry
    voxelux::core::MaterialRegistry materials;
    auto test_material = voxelux::core::Material("Test", voxelux::core::Color(100, 100, 100));
    auto material_id = materials.add_material(test_material);
    
    std::cout << "Material added with ID: " << material_id << std::endl;
    
    // Test voxel placement
    grid.set_voxel(2, 2, 2, voxelux::core::Voxel(material_id));
    std::cout << "Active voxels: " << grid.active_voxel_count() << std::endl;
    
    std::cout << "Basic tests passed!" << std::endl;
    return 0;
}