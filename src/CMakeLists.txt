# Core library modules
add_subdirectory(core)

# Modern Qt+OpenGL executable (unified application)
if(Qt6_FOUND AND OPENGL_FOUND)
    message(STATUS "Building modern Qt+OpenGL application with Blender-style viewport")
    
    # Enable Qt MOC
    set(CMAKE_AUTOMOC ON)
    
    add_executable(voxelux 
        voxelux_opengl.cpp 
        theme.cpp
        typography.cpp
        viewport_widget.cpp
        viewport_nav_widget.cpp
        text_renderer.cpp
    )
    
    target_link_libraries(voxelux 
        Qt6::Core
        Qt6::Widgets 
        Qt6::Gui
        Qt6::OpenGL
        Qt6::OpenGLWidgets
        ${OPENGL_LIBRARIES}
        voxelux_core
    )
    
    target_compile_features(voxelux PRIVATE cxx_std_20)
    target_include_directories(voxelux PRIVATE ${OPENGL_INCLUDE_DIR})
    target_compile_definitions(voxelux PRIVATE VOXELUX_USE_OPENGL)
    
    # Set Qt6 path for runtime on macOS
    if(APPLE)
        set_target_properties(voxelux PROPERTIES
            INSTALL_RPATH "/opt/homebrew/lib")
    endif()
    
else()
    message(FATAL_ERROR "Modern Qt+OpenGL application requires Qt6 and OpenGL - dependencies not found")
endif()