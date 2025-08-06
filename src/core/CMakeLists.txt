# Core library sources (clean, minimal)
set(CORE_SOURCES
    voxel_grid.cpp
)

# Create core library
add_library(voxelux_core STATIC ${CORE_SOURCES})

# Include directories
target_include_directories(voxelux_core 
    PUBLIC 
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Compile features
target_compile_features(voxelux_core PUBLIC cxx_std_20)