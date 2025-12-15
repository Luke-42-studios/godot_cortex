// ECSWorldSystem.h - Enhanced for Godot integration
#pragma once

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node.hpp>
#include <flecs.h>

using namespace godot;

// =============================================================================
// ECS Components - Registered with Flecs for proper naming and queries
// =============================================================================

// Links an ECS entity to a Godot Node
struct CGodotNode {
    Node* ptr = nullptr;
    uint64_t instance_id = 0;  // For validity checking
};

// Depth in the scene tree hierarchy
struct CNodeDepth {
    size_t value = 0;
};

// Scene tree identifier
struct CTreeId {
    uint32_t value = 0;
};

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

    // Debug: Print all entities and their components
    void print_state() const;

private:
    void _initialize();
    void _shutdown();

public:  
    static PECSContext* create_global_instance();
    static void destroy_global_instance();

private:
    static void _bind_methods();
};