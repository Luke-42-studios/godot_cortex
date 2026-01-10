// examples/domains/input.cpp
// Input domain module - captures player input
//
// Runs in Phase_Input (event-driven).
// Simple domain - just reads Godot input into ECS components.

#include "polaris/core/Runtime.h"
#include "polaris/components/Gd.h"
#include <godot_cpp/classes/input.hpp>

using namespace Polaris;

namespace Input {

// ============================================================================
// Components
// ============================================================================

struct InputState {
    godot::Vector2 move;
    godot::Vector2 look;
    bool jump;
    bool attack;
    bool interact;
};

// ============================================================================
// Systems (static = private to this file)
// ============================================================================
// Simple .each() callback - just components, no entity or iter needed

static void capture_player_input(InputState& input) {
    godot::Input* gd = godot::Input::get_singleton();
    if (!gd) return;

    // Movement (WASD / stick)
    input.move = gd->get_vector(
        "move_left", "move_right", "move_forward", "move_back"
    );

    // Look (mouse / right stick)
    input.look = gd->get_last_mouse_velocity() * 0.001f;

    // Actions (just-pressed for single triggers)
    input.jump = gd->is_action_just_pressed("jump");
    input.attack = gd->is_action_just_pressed("attack");
    input.interact = gd->is_action_just_pressed("interact");
}

// ============================================================================
// Initialization - Single entry point
// ============================================================================

void init(Runtime* rt) {
    flecs::world& w = rt->world();
    flecs::entity input_phase = rt->get_phase(Phase_Input).id;

    // Only process player-controlled entities
    w.system<InputState>("CapturePlayerInput")
        .kind(input_phase)
        .with<Tag::Player>()
        .each(capture_player_input);
}

} // namespace Input
