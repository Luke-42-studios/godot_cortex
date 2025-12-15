#ifndef POLARIS_INIT_H
#define POLARIS_INIT_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "CPolaris.h"
#include "system/PECSContext.h"
#include "system/SNodeWatcher.h"
#include "node/CNode.h"

namespace godot {

// ============================================================================
// Module Initialization - Polaris Framework
// HOT PATH: Called during engine startup, optimize for reliability over speed
// ============================================================================

/// @brief Registers all Polaris framework classes with Godot's class system
/// PERF: Single registration call batches all type registrations together
inline void polaris_register_classes() {
    // Register all classes with Godot's ClassDB
    GDREGISTER_CLASS(PECSContext);
    GDREGISTER_CLASS(SNodeWatcher);
    GDREGISTER_CLASS(CPolaris);
    GDREGISTER_CLASS(Context);
    GDREGISTER_CLASS(CNode);

    UtilityFunctions::print("[Polaris] Framework classes registered");

    // Create single entry point - CPolaris owns and initializes all subsystems
    // This creates PECSContext, SNodeWatcher, wires callbacks, and binds to scene tree
    CPolaris::create_global_instance();
}

// ============================================================================
// Call this in your game's uninitialize function at MODULE_INITIALIZATION_LEVEL_SCENE
// ============================================================================
inline void polaris_unregister_classes() {
    // Single cleanup point - CPolaris destroys all subsystems in correct order
    CPolaris::destroy_global_instance();
}

} // namespace godot

#endif // POLARIS_INIT_H