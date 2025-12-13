#ifndef FLECS_WORLD_H
#define FLECS_WORLD_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>

// Forward declare flecs types (from flecs.h)
typedef struct ecs_world_t ecs_world_t;

namespace godot {

/**
 * FlecsWorld - Singleton system for Flecs ECS integration
 *
 * This class manages the Flecs world instance and provides access to ECS
 * functionality from both C++ and GDScript. It is initialized when the
 * GDExtension loads and cleaned up when it unloads.
 *
 * Access from GDScript:
 *   var flecs = Engine.get_singleton("FlecsWorld")
 *   flecs.progress(delta)
 *
 * Access from C++:
 *   FlecsWorld* flecs = FlecsWorld::get_singleton();
 *   flecs->get_world(); // Get raw ecs_world_t*
 */
class FlecsWorld : public Object {
    GDCLASS(FlecsWorld, Object);

private:
    static FlecsWorld* singleton;
    ecs_world_t* world = nullptr;
    bool initialized = false;

protected:
    static void _bind_methods();

public:
    FlecsWorld();
    ~FlecsWorld();

    // Singleton access
    static FlecsWorld* get_singleton();

    // === Lifecycle ===

    // Initialize the Flecs world (called automatically on module init)
    void initialize();

    // Shutdown the Flecs world (called automatically on module uninit)
    void shutdown();

    // Check if initialized
    bool is_initialized() const;

    // === Core ECS Operations ===

    // Progress the world by delta time (call from _process)
    // Returns true if the world should continue running
    bool progress(double delta);

    // Get the raw Flecs world pointer (for C++ use)
    ecs_world_t* get_world() const;

    // === World Stats ===

    // Get number of entities in the world
    int64_t get_entity_count() const;

    // Get world info as a dictionary
    Dictionary get_world_info() const;

    // === Utility ===

    // Set target FPS for Flecs (0 = variable timestep)
    void set_target_fps(double fps);

    // Get current target FPS
    double get_target_fps() const;

    // Enable/disable world (pauses all systems when disabled)
    void set_enabled(bool enabled);
    bool is_enabled() const;
};

} // namespace godot

#endif // FLECS_WORLD_H
