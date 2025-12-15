#ifndef POLARIS_INIT_H
#define POLARIS_INIT_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "system/ecs_context.h"
#include "system/node_watcher.h"

namespace godot {

// ============================================================================
// Module Initialization - Polaris Framework
// HOT PATH: Called during engine startup, optimize for reliability over speed
// ============================================================================

/// @brief Registers all Polaris framework classes with Godot's class system
/// PERF: Single registration call batches all type registrations together
inline void polaris_register_classes() {
    // Register core ECS systems first (dependencies)
    GDREGISTER_CLASS(PECSContext);
    GDREGISTER_CLASS(NodeWatcher);
        
    UtilityFunctions::print("[Polaris] Framework classes registered");

    // PERF: Singleton creation order matters - create dependencies first
    // Create the global ECS context instance (HOT: singleton lifetime management)
    PECSContext::create_global_instance();

    // Create NodeWatcher as a supporting service
    NodeWatcher::create_global_instance();
}

// ============================================================================
// Call this in your game's uninitialize function at MODULE_INITIALIZATION_LEVEL_SCENE
// ============================================================================
inline void polaris_unregister_classes() {
    PECSContext::destroy_global_instance();
    NodeWatcher::destroy_global_instance();
}

} // namespace godot`

#endif // POLARIS_INIT_H