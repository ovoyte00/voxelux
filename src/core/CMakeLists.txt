# Copyright (C) 2024 Voxelux
# 
# This software and its source code are proprietary and confidential.
# All rights reserved. No part of this software may be reproduced,
# distributed, or transmitted in any form or by any means without
# prior written permission from Voxelux.
# 
# Professional voxel core library build configuration.

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