#ifndef POLARIS_INIT_H
#define POLARIS_INIT_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "Engine.h"

namespace godot {


// ============================================================================
// Module Initialization - Polaris Framework
// HOT PATH: Called during engine startup, optimize for reliability over speed
// ============================================================================

/// @brief Registers all Polaris framework classes with Godot's class system
/// PERF: Single registration call batches all type registrations together
inline void polaris_register_classes() {
    // Register all classes with Godot's ClassDB
    // Note: Context is now built into Godot's Node class - no CNode needed
    GDREGISTER_CLASS(Polaris::PolarisEngine);

    UtilityFunctions::print("[Polaris] Framework classes registered");

    Polaris::PolarisEngine::create_global_instance();
}

// ============================================================================
// Call this in your game's uninitialize function at MODULE_INITIALIZATION_LEVEL_SCENE
// ============================================================================
inline void polaris_unregister_classes() {
    // Single cleanup point - PolarisEngine destroys all subsystems in correct order
    Polaris::PolarisEngine::destroy_global_instance();
}

} // namespace godot

#endif // POLARIS_INIT_H