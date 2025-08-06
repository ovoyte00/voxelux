# Tests configuration
# For now, create a placeholder - will add proper tests later

# Placeholder test to ensure CMake works
add_executable(test_placeholder test_placeholder.cpp)
target_link_libraries(test_placeholder voxelux_core)
target_compile_features(test_placeholder PRIVATE cxx_std_20)

# Add tests when we have a test framework
# add_test(NAME test_placeholder COMMAND test_placeholder)