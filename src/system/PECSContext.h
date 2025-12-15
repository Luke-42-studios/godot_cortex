// ECSWorldSystem.h - Enhanced for Godot integration
#pragma once

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/classes/engine.hpp> 
#include <flecs.h>

using namespace godot;

// CACHE: Use forward declarations to minimize include dependencies
namespace flecs { class world; }

/// @brief Polaris Context for ECS World management and Godot integration
/// HOT PATH: Provides direct C++ access to ECS systems while maintaining Godot compatibility
class PECSContext : public Object {
    GDCLASS(PECSContext, Object)

private:
    static inline PECSContext* singleton_instance = nullptr;
    std::unique_ptr<flecs::world> m_world;  // PERF: Unique ptr prevents accidental copies
    
    // Prevent copying (singleton pattern) - operator= already handled by GDCLASS
    PECSContext(const PECSContext&) = delete;

public:
    /** @brief Fast C++ access to singleton instance */
    static PECSContext* get_singleton() noexcept {
        return singleton_instance;
    }

    // Constructor/Destructor  
    PECSContext();
    ~PECSContext();

    // Core ECS world access
    [[nodiscard]] flecs::world& get_world() noexcept { 
        return *m_world; 
    }
    
    [[nodiscard]] const flecs::world& get_world() const noexcept { 
        return *m_world; 
    }

private:
    void _initialize();
    void _shutdown();

public:  
    static PECSContext* create_global_instance();
    static void destroy_global_instance();

private:
    static void _bind_methods();
};