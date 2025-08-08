# Platform-specific native helpers

set(PLATFORM_SOURCES)
set(PLATFORM_HEADERS
    ${PROJECT_SOURCE_DIR}/include/voxelux/platform/native_input.h
)

# Platform-specific implementations
if(APPLE)
    list(APPEND PLATFORM_SOURCES native_input_macos.mm)
    set_source_files_properties(native_input_macos.mm PROPERTIES 
        COMPILE_FLAGS "-x objective-c++"
    )
elseif(WIN32)
    # TODO: Add Windows implementation
    list(APPEND PLATFORM_SOURCES native_input_stub.cpp)
else()
    # Linux/Unix - use stub for now
    list(APPEND PLATFORM_SOURCES native_input_stub.cpp)
endif()

# Create platform library
add_library(voxelux_platform STATIC
    ${PLATFORM_SOURCES}
    ${PLATFORM_HEADERS}
)

target_include_directories(voxelux_platform PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# Link with required libraries
target_link_libraries(voxelux_platform PUBLIC
    glfw
)

if(APPLE)
    target_link_libraries(voxelux_platform PUBLIC
        "-framework Cocoa"
        "-framework Carbon"
    )
endif()

# Set C++ standard
target_compile_features(voxelux_platform PUBLIC cxx_std_20)