#pragma once

#include "simple_event.h"
#include "vector3.h"
#include "voxel.h"

namespace voxelux::core::events {

// Input Events
class KeyEvent : public SimpleEvent {
public:
    enum class Action { Press, Release, Repeat };
    
    KeyEvent(int key, int scancode, Action action, int mods)
        : key_(key), scancode_(scancode), action_(action), mods_(mods) {}
    
    int key() const { return key_; }
    int scancode() const { return scancode_; }
    Action action() const { return action_; }
    int mods() const { return mods_; }

private:
    int key_;
    int scancode_;
    Action action_;
    int mods_;
};

class MouseButtonEvent : public SimpleEvent {
public:
    enum class Action { Press, Release };
    
    MouseButtonEvent(int button, Action action, int mods, double x, double y)
        : button_(button), action_(action), mods_(mods), x_(x), y_(y) {}
    
    int button() const { return button_; }
    Action action() const { return action_; }
    int mods() const { return mods_; }
    double x() const { return x_; }
    double y() const { return y_; }

private:
    int button_;
    Action action_;
    int mods_;
    double x_, y_;
};

class MouseMoveEvent : public SimpleEvent {
public:
    MouseMoveEvent(double x, double y, double dx, double dy)
        : x_(x), y_(y), dx_(dx), dy_(dy) {}
    
    double x() const { return x_; }
    double y() const { return y_; }
    double dx() const { return dx_; }
    double dy() const { return dy_; }

private:
    double x_, y_;
    double dx_, dy_;
};

class ScrollEvent : public SimpleEvent {
public:
    ScrollEvent(double x_offset, double y_offset)
        : x_offset_(x_offset), y_offset_(y_offset) {}
    
    double x_offset() const { return x_offset_; }
    double y_offset() const { return y_offset_; }

private:
    double x_offset_, y_offset_;
};

// Window Events
class WindowResizeEvent : public SimpleEvent {
public:
    WindowResizeEvent(int width, int height)
        : width_(width), height_(height) {}
    
    int width() const { return width_; }
    int height() const { return height_; }

private:
    int width_, height_;
};

class WindowCloseEvent : public SimpleEvent {
public:
    WindowCloseEvent() = default;
};

// Voxel Editor Events
class VoxelPlacedEvent : public SimpleEvent {
public:
    VoxelPlacedEvent(const Vector3i& position, const Voxel& voxel)
        : position_(position), voxel_(voxel) {}
    
    const Vector3i& position() const { return position_; }
    const Voxel& voxel() const { return voxel_; }

private:
    Vector3i position_;
    Voxel voxel_;
};

class VoxelRemovedEvent : public SimpleEvent {
public:
    VoxelRemovedEvent(const Vector3i& position, const Voxel& old_voxel)
        : position_(position), old_voxel_(old_voxel) {}
    
    const Vector3i& position() const { return position_; }
    const Voxel& old_voxel() const { return old_voxel_; }

private:
    Vector3i position_;
    Voxel old_voxel_;
};

class MaterialChangedEvent : public SimpleEvent {
public:
    MaterialChangedEvent(uint32_t material_id, const Material& old_material, const Material& new_material)
        : material_id_(material_id), old_material_(old_material), new_material_(new_material) {}
    
    uint32_t material_id() const { return material_id_; }
    const Material& old_material() const { return old_material_; }
    const Material& new_material() const { return new_material_; }

private:
    uint32_t material_id_;
    Material old_material_;
    Material new_material_;
};

// Grid Events
class GridResizedEvent : public SimpleEvent {
public:
    GridResizedEvent(const Vector3i& old_size, const Vector3i& new_size)
        : old_size_(old_size), new_size_(new_size) {}
    
    const Vector3i& old_size() const { return old_size_; }
    const Vector3i& new_size() const { return new_size_; }

private:
    Vector3i old_size_;
    Vector3i new_size_;
};

class GridClearedEvent : public SimpleEvent {
public:
    GridClearedEvent() = default;
};

// Application Events
class ApplicationStartEvent : public SimpleEvent {
public:
    ApplicationStartEvent() = default;
};

class ApplicationShutdownEvent : public SimpleEvent {
public:
    ApplicationShutdownEvent() = default;
};

// Tool Events
class ToolChangedEvent : public SimpleEvent {
public:
    enum class ToolType { 
        Brush, 
        Eraser, 
        Picker, 
        Select, 
        Move, 
        Rotate, 
        Scale 
    };
    
    ToolChangedEvent(ToolType old_tool, ToolType new_tool)
        : old_tool_(old_tool), new_tool_(new_tool) {}
    
    ToolType old_tool() const { return old_tool_; }
    ToolType new_tool() const { return new_tool_; }

private:
    ToolType old_tool_;
    ToolType new_tool_;
};

// Project Events
class ProjectCreatedEvent : public SimpleEvent {
public:
    enum class ProjectType { 
        Generic, 
        MinecraftJava, 
        MinecraftBedrock, 
        Unity, 
        Unreal 
    };
    
    ProjectCreatedEvent(ProjectType type, const std::string& name)
        : type_(type), name_(name) {}
    
    ProjectType type() const { return type_; }
    const std::string& name() const { return name_; }

private:
    ProjectType type_;
    std::string name_;
};

class ProjectLoadedEvent : public SimpleEvent {
public:
    ProjectLoadedEvent(const std::string& path)
        : path_(path) {}
    
    const std::string& path() const { return path_; }

private:
    std::string path_;
};

class ProjectSavedEvent : public SimpleEvent {
public:
    ProjectSavedEvent(const std::string& path)
        : path_(path) {}
    
    const std::string& path() const { return path_; }

private:
    std::string path_;
};

}