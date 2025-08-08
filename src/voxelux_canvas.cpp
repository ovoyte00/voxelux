/*
 * Copyright (C) 2024 Voxelux
 * 
 * This software and its source code are proprietary and confidential.
 * All rights reserved. No part of this software may be reproduced,
 * distributed, or transmitted in any form or by any means without
 * prior written permission from Voxelux.
 * 
 * Voxelux - Professional voxel editing application with Canvas UI
 */

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "canvas_ui/canvas_window.h"
#include "canvas_ui/canvas_renderer.h"
#include "canvas_ui/event_router.h"
#include "canvas_ui/canvas_region.h"

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

using namespace voxel_canvas;

class VoxeluxApplication {
public:
    VoxeluxApplication() {
        window_ = std::make_unique<CanvasWindow>(1600, 1000, "Voxelux - Professional Voxel Editor");
    }
    
    ~VoxeluxApplication() {
        shutdown();
    }
    
    bool initialize() {
        std::cout << "Initializing Voxelux with Canvas UI..." << std::endl;
        
        if (!window_->initialize()) {
            std::cerr << "Failed to initialize Canvas window" << std::endl;
            return false;
        }
        
        setup_event_handlers();
        setup_default_workspace();
        
        std::cout << "Voxelux initialized successfully" << std::endl;
        return true;
    }
    
    void run() {
        std::cout << "Starting Voxelux main loop..." << std::endl;
        std::cout << "Press ESC or close window to exit." << std::endl;
        
        while (!window_->should_close()) {
            window_->poll_events();
            window_->render_frame();
            window_->swap_buffers();
            
            // Small delay to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        std::cout << "Voxelux main loop ended" << std::endl;
    }
    
    void shutdown() {
        if (window_) {
            window_->shutdown();
        }
    }
    
private:
    void setup_event_handlers() {
        auto* event_router = window_->get_event_router();
        
        // Add global shortcut handler
        auto global_shortcuts = std::make_shared<GlobalShortcutHandler>();
        
        // Register some basic shortcuts (using GLFW key codes)
        global_shortcuts->register_shortcut(256, 0, [this]() { // GLFW_KEY_ESCAPE
            std::cout << "ESC pressed - closing application" << std::endl;
            glfwSetWindowShouldClose(window_->get_glfw_handle(), GLFW_TRUE);
        });
        
        global_shortcuts->register_shortcut(81, 1024, [this]() { // Q key + GLFW_MOD_SUPER (Cmd on macOS)
            std::cout << "Quit shortcut pressed" << std::endl;
            glfwSetWindowShouldClose(window_->get_glfw_handle(), GLFW_TRUE);
        });
        
        event_router->add_handler(global_shortcuts);
        
        // Add region interaction handler
        auto region_handler = std::make_shared<RegionInteractionHandler>(window_.get());
        event_router->add_handler(region_handler);
    }
    
    void setup_default_workspace() {
        auto* workspace_manager = window_->get_workspace_manager();
        
        // Set up default "3D Editing" workspace
        workspace_manager->set_active_workspace("3D Editing");
        
        std::cout << "Active workspace: " << workspace_manager->get_active_workspace() << std::endl;
        
        // The region manager already creates a default 3D viewport
        // TODO: Later we'll add more complex layout configurations
    }
    
    std::unique_ptr<CanvasWindow> window_;
};

int main(int argc, char* argv[]) {
    std::cout << "Voxelux v0.1.0 - Professional Voxel Editor" << std::endl;
    std::cout << "Copyright (C) 2024 Voxelux. All rights reserved." << std::endl;
    std::cout << std::endl;
    
    try {
        VoxeluxApplication app;
        
        if (!app.initialize()) {
            std::cerr << "Failed to initialize Voxelux application" << std::endl;
            return 1;
        }
        
        app.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 1;
    }
    
    std::cout << "Voxelux exited successfully" << std::endl;
    return 0;
}