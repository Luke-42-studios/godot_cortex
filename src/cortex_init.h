#ifndef CORTEX_INIT_H
#define CORTEX_INIT_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "system/ecs_context.h"
#include "system/node_watcher_system.h"

namespace godot {
// ============================================================================
// Call this in your game's initialize function at MODULE_INITIALIZATION_LEVEL_SCENE
// ============================================================================
inline void cortex_register_classes() {
    GDREGISTER_CLASS(ECSContext);
    GDREGISTER_CLASS(NodeWatcherSystem);
        
    UtilityFunctions::print("[Cortex] Framework classes registered");

    // Create the global instance
    ECSContext::create_global_instance();

    // Create a global instance that lives for the whole engine run‑time.
    NodeWatcherSystem::create_global_instance();
}

// ============================================================================
// Call this in your game's uninitialize function at MODULE_INITIALIZATION_LEVEL_SCENE
// ============================================================================
inline void cortex_unregister_classes() {
    ECSContext::destroy_global_instance();
    NodeWatcherSystem::destroy_global_instance();
}

} // namespace godot`

#endif // CORTEX_INIT_H