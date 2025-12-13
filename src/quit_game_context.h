#ifndef QUIT_GAME_CONTEXT_H
#define QUIT_GAME_CONTEXT_H

#include "clay_button_node.h"
#include <godot_cpp/classes/scene_tree.hpp>

namespace godot {

// ============================================================================
// QuitGameContext - Prints "Quitting Game!" when button is pressed
// ============================================================================
class QuitGameContext : public ClayButtonContext {
    GDCLASS(QuitGameContext, ClayButtonContext);

    // Context-specific properties
    GD_PROPERTY(bool, confirm_quit, true)
    GD_PROPERTY(String, quit_message, "Thanks for playing!")

protected:
    static void _bind_methods() {
        GD_BIND_PROPERTY(QuitGameContext, bool, confirm_quit);
        GD_BIND_PROPERTY(QuitGameContext, String, quit_message);

        GD_BIND_SIGNAL(game_quitting);
    }

public:
    QuitGameContext() {}
    ~QuitGameContext() {}

    void ctx_ready() override {
        UtilityFunctions::print("[QuitGameContext] Ready! Confirm: ", _confirm_quit ? "yes" : "no");
    }

    // =========================================================================
    // Button interface implementation
    // =========================================================================
    void on_pressed() override {
        UtilityFunctions::print("========================================");
        UtilityFunctions::print("  QUITTING GAME!");
        UtilityFunctions::print("  Message: ", _quit_message);
        if (_confirm_quit) {
            UtilityFunctions::print("  (Would show confirmation dialog)");
        }
        UtilityFunctions::print("========================================");

        emit_signal("game_quitting");

        // Access button through get_node<T>() - generic and typed!
        ClayButtonNode* btn = get_node<ClayButtonNode>();
        if (btn) {
            UtilityFunctions::print("  Button label: ", btn->get_label());

            // Could actually quit:
            // btn->get_tree()->quit();
        }
    }

    void on_hover_enter() override {
        UtilityFunctions::print("[QuitGame] Are you sure you want to quit?");
    }

    void on_hover_exit() override {
        UtilityFunctions::print("[QuitGame] Phew, staying!");
    }
};

} // namespace godot

#endif // QUIT_GAME_CONTEXT_H
