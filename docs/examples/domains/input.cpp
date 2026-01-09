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

static void capture_player_input(flecs::iter& it, InputState* input) {
    godot::Input* godot_input = godot::Input::get_singleton();

    for (auto i : it) {
        // Movement (WASD / stick)
        input[i].move = godot_input->get_vector(
            "move_left", "move_right", "move_forward", "move_back"
        );

        // Look (mouse / right stick)
        input[i].look = godot_input->get_last_mouse_velocity() * 0.001f;

        // Actions (just-pressed for single triggers)
        input[i].jump = godot_input->is_action_just_pressed("jump");
        input[i].attack = godot_input->is_action_just_pressed("attack");
        input[i].interact = godot_input->is_action_just_pressed("interact");
    }
}

// ============================================================================
// Initialization - Single entry point
// ============================================================================

void init(Runtime* rt) {
    flecs::world& world = rt->world;
    flecs::entity input_phase = rt->phases[Phase_Input].id;

    // Only process player-controlled entities
    world.system<InputState>("CapturePlayerInput")
        .kind(input_phase)
        .with<Tag::Player>()
        .iter(capture_player_input);
}

} // namespace Input
