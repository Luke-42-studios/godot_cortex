#ifndef POLARIS_ECS_WORLD_H
#define POLARIS_ECS_WORLD_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <flecs.h>
#include <mutex>

namespace Polaris {

using namespace godot;

// =============================================================================
// ECSWorld - Godot singleton wrapping flecs::world
// =============================================================================
//
// Provides access to the flecs ECS world from anywhere in the codebase.
// Registered as a Godot singleton for easy access from GDScript.
//
// USAGE (C++):
//   auto& world = ECSWorld::get()->world();
//   flecs::entity e = world.entity();
//
// USAGE (GDScript):
//   var ecs = Engine.get_singleton("ECSWorld")
//   print(ecs.get_entity_count())
//
// =============================================================================

class ECSWorld : public Object {
    GDCLASS(ECSWorld, Object)

private:
    static inline ECSWorld* singleton_instance = nullptr;
    flecs::world m_world;

    ECSWorld(const ECSWorld&) = delete;

protected:
    static void _bind_methods();

public:
    ECSWorld();
    ~ECSWorld();

    // =========================================================================
    // World Access
    // =========================================================================

    /// Get the flecs world (C++ only)
    flecs::world& world() { return m_world; }
    const flecs::world& world() const { return m_world; }

    // =========================================================================
    // GDScript API
    // =========================================================================

    /// Get current entity count
    int get_entity_count() const;

    /// Progress the world (manual tick - normally called by PipelineNode)
    void progress(float delta);

    // =========================================================================
    // Singleton
    // =========================================================================

    static ECSWorld* get() noexcept { return singleton_instance; }
    static ECSWorld* create_global_instance();
    static void destroy_global_instance();
};

} // namespace Polaris

#endif // POLARIS_ECS_WORLD_H
