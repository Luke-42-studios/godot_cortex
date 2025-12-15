// ECSWorldSystem.h - Enhanced for Godot integration
#pragma once

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/classes/engine.hpp> 
#include <flecs.h>

using namespace godot;

class ECSContext : public Object {
    GDCLASS(ECSContext, Object)

private:
    static inline ECSContext* singleton_instance = nullptr;
    flecs::world ecs_world;
    
    // Prevent copying
    ECSContext(const ECSContext&) = delete;
   // ECSContext& operator=(const ECSContext&) = delete;

public:
    // Fast C++ access (avoids Engine string lookup)
    static ECSContext* get_singleton() noexcept {
        return singleton_instance;
    }

    // Constructor/Destructor  
    ECSContext();
    ~ECSContext();

    // Core ECS world access
    [[nodiscard]] const flecs::world& get_world() const noexcept { 
        return ecs_world; 
    }
    
    [[nodiscard]] flecs::world& get_world_mutable() noexcept { 
        return ecs_world; 
    }

    // Godot lifecycle integration
    void _initialize();
    void _shutdown();

private:
    static void _bind_methods();

public:  
    // Static methods for module integration
    static ECSContext* create_global_instance();
    static void destroy_global_instance();
};
