#ifndef POLARIS_INIT_H
#define POLARIS_INIT_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "Engine.h"
#include "context/ECSWorld.h"
#include "system/NodeWatcher.h"
#include "system/TickerNode.h"

namespace godot {

// ============================================================================
// System Registration Callback
// ============================================================================

/// @brief Callback type for game system registration
/// Called by PolarisEngine after world is initialized
using SystemRegistrationFn = void(*)(flecs::world&);

namespace detail {
    inline SystemRegistrationFn s_system_callback = nullptr;
}

/// @brief Set the callback that registers game ECS systems
/// Must be called BEFORE polaris_register_classes()
/// @param fn Function that registers all game systems with the world
inline void polaris_set_system_callback(SystemRegistrationFn fn) {
    detail::s_system_callback = fn;
}

/// @brief Called by PolarisEngine when world is ready
/// @internal
inline void polaris_invoke_system_callback(flecs::world& world) {
    if (detail::s_system_callback) {
        detail::s_system_callback(world);
    }
}

// ============================================================================
// Module Initialization - Polaris Framework
// HOT PATH: Called during engine startup, optimize for reliability over speed
// ============================================================================

/// @brief Registers all Polaris framework classes with Godot's class system
/// PERF: Single registration call batches all type registrations together
inline void polaris_register_classes() {
    // Register all classes with Godot's ClassDB
    // Note: Context is now built into Godot's Node class - no CNode needed
    GDREGISTER_CLASS(Polaris::Context::ECSWorld);
    GDREGISTER_CLASS(Polaris::System::NodeWatcher);
    GDREGISTER_CLASS(Polaris::System::TickerNode);
    GDREGISTER_CLASS(Polaris::PolarisEngine);

    UtilityFunctions::print("[Polaris] Framework classes registered");

    // Create single entry point - PolarisEngine owns and initializes all subsystems
    // This creates ECSWorld, NodeWatcher, wires callbacks, and binds to scene tree
    // Also calls the system registration callback if set
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