#ifndef CORTEX_INIT_H
#define CORTEX_INIT_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "cnode.h"
#include "clay_button_node.h"
#include "start_game_context.h"
#include "quit_game_context.h"
#include "option_menu_context.h"
#include "flecs_world.h"

namespace godot {

// Static singleton pointer - managed by game project
inline FlecsWorld* g_flecs_world_singleton = nullptr;

// ============================================================================
// Call this in your game's initialize function at MODULE_INITIALIZATION_LEVEL_SCENE
// ============================================================================
inline void cortex_register_classes() {
    GDREGISTER_CLASS(NodeContext);
    GDREGISTER_CLASS(CNode);
    GDREGISTER_CLASS(ClayButtonContext);
    GDREGISTER_CLASS(ClayButtonNode);
    GDREGISTER_CLASS(StartGameContext);
    GDREGISTER_CLASS(QuitGameContext);
    GDREGISTER_CLASS(OptionMenuContext);
    GDREGISTER_CLASS(FlecsWorld);

    // Initialize Flecs singleton
    g_flecs_world_singleton = memnew(FlecsWorld);
    Engine::get_singleton()->register_singleton("FlecsWorld", g_flecs_world_singleton);
    g_flecs_world_singleton->initialize();

    UtilityFunctions::print("[Cortex] Framework classes registered");
}

// ============================================================================
// Call this in your game's uninitialize function at MODULE_INITIALIZATION_LEVEL_SCENE
// ============================================================================
inline void cortex_unregister_classes() {
    if (g_flecs_world_singleton) {
        Engine::get_singleton()->unregister_singleton("FlecsWorld");
        memdelete(g_flecs_world_singleton);
        g_flecs_world_singleton = nullptr;
    }
}

} // namespace godot

#endif // CORTEX_INIT_H
