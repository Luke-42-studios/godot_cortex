#ifndef START_GAME_CONTEXT_H
#define START_GAME_CONTEXT_H

#include "clay_button_node.h"

namespace godot {

// ============================================================================
// StartGameContext - Prints "Starting Game!" when button is pressed
// ============================================================================
class StartGameContext : public ClayButtonContext {
    GDCLASS(StartGameContext, ClayButtonContext);

    // Context-specific properties
    GD_PROPERTY(String, game_scene, "res://scenes/game.tscn")
    GD_PROPERTY(bool, show_loading, true)

protected:
    static void _bind_methods() {
        GD_BIND_PROPERTY(StartGameContext, String, game_scene);
        GD_BIND_PROPERTY(StartGameContext, bool, show_loading);

        GD_BIND_SIGNAL(game_starting);
    }

public:
    StartGameContext() {}
    ~StartGameContext() {}

    void ctx_ready() override {
        UtilityFunctions::print("[StartGameContext] Ready! Will load: ", _game_scene);
    }

    // =========================================================================
    // Button interface implementation
    // =========================================================================
    void on_pressed() override {
        UtilityFunctions::print("========================================");
        UtilityFunctions::print("  STARTING GAME!");
        UtilityFunctions::print("  Loading scene: ", _game_scene);
        if (_show_loading) {
            UtilityFunctions::print("  (Loading screen enabled)");
        }
        UtilityFunctions::print("========================================");

        emit_signal("game_starting");

        // Access button through get_node<T>() - generic and typed!
        ClayButtonNode* btn = get_node<ClayButtonNode>();
        if (btn) {
            UtilityFunctions::print("  Button label: ", btn->get_label());
            UtilityFunctions::print("  Button hovered: ", btn->is_button_hovered());
        }
    }

    void on_hover_enter() override {
        UtilityFunctions::print("[StartGame] Ready to start!");
    }

    void on_hover_exit() override {
        UtilityFunctions::print("[StartGame] Hover exit");
    }
};

} // namespace godot

#endif // START_GAME_CONTEXT_H
