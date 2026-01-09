// systems/movement.cpp
// Complete movement system example for Polaris Engine

#include "polaris/core/Runtime.h"
#include "components/Data.h"
#include "components/Tags.h"
#include "components/Gd.h"

namespace Polaris::Systems {

// ============================================================================
// PHASE: Input
// ============================================================================

void input_capture(flecs::iter& it, InputState* input) {
    Input* godot_input = Input::get_singleton();

    for (auto i : it) {
        input[i].move = godot_input->get_vector(
            "move_left", "move_right", "move_forward", "move_back"
        );
        input[i].look = godot_input->get_last_mouse_velocity() * 0.001f;
        input[i].jump = godot_input->is_action_just_pressed("jump");
        input[i].attack = godot_input->is_action_just_pressed("attack");
    }
}

// ============================================================================
// PHASE: Physics (Game Logic)
// ============================================================================

void player_movement(flecs::iter& it,
                     InputState* input,
                     MoveSpeed* speed,
                     Velocity* vel,
                     Gd::Node3D* node)
{
    for (auto i : it) {
        if (!node[i].is_valid()) continue;

        // Get camera-relative directions
        Vector3 forward = node[i].get_forward();
        Vector3 right = node[i].get_right();
        forward.y = 0; forward.normalize();
        right.y = 0; right.normalize();

        // Apply input to velocity
        Vector3 dir = forward * input[i].move.y + right * input[i].move.x;
        vel[i].linear.x = dir.x * speed[i].value;
        vel[i].linear.z = dir.z * speed[i].value;

        // Handle jump
        if (input[i].jump && it.entity(i).has<Tag::Grounded>()) {
            vel[i].linear.y = 5.0f;
        }
    }
}

void apply_gravity(flecs::iter& it, Velocity* vel) {
    const float gravity = -9.8f;
    float dt = it.delta_time();

    for (auto i : it) {
        flecs::entity e = it.entity(i);
        if (!e.has<Tag::Grounded>()) {
            vel[i].linear.y += gravity * dt;
        }
    }
}

void apply_velocity(flecs::iter& it,
                    Velocity* vel,
                    Gd::CharacterBody3D* body)
{
    for (auto i : it) {
        if (!body[i].is_valid()) continue;

        body[i].ptr->set_velocity(vel[i].linear);
        body[i].ptr->move_and_slide();

        // Read back actual velocity after collision
        vel[i].linear = body[i].ptr->get_velocity();
    }
}

void check_grounded(flecs::iter& it, Gd::CharacterBody3D* body) {
    for (auto i : it) {
        if (!body[i].is_valid()) continue;

        flecs::entity e = it.entity(i);
        if (body[i].ptr->is_on_floor()) {
            e.add<Tag::Grounded>();
        } else {
            e.remove<Tag::Grounded>();
        }
    }
}

// ============================================================================
// PHASE: Process (Visual Updates)
// ============================================================================

void update_visual_position(flecs::iter& it,
                           Velocity* vel,
                           Gd::Node3D* node)
{
    float dt = it.delta_time();

    for (auto i : it) {
        if (!node[i].is_valid()) continue;

        // Interpolate position for smooth visuals
        Vector3 pos = node[i].get_position();
        pos += vel[i].linear * dt;
        node[i].set_position(pos);
    }
}

// ============================================================================
// Registration
// ============================================================================

void register_movement_systems(Runtime* rt) {
    flecs::entity input = rt->phases[Phase_Input].id;
    flecs::entity physics = rt->phases[Phase_Physics].id;
    flecs::entity process = rt->phases[Phase_Process].id;

    // Input phase - capture player input
    rt->world.system<InputState>("InputCapture")
        .kind(input)
        .with<Tag::Player>()
        .iter(input_capture);

    // Physics phase - game logic (runs in order)
    rt->world.system<InputState, MoveSpeed, Velocity, Gd::Node3D>
        ("PlayerMovement")
        .kind(physics)
        .with<Tag::Player>()
        .iter(player_movement);

    rt->world.system<Velocity>("ApplyGravity")
        .kind(physics)
        .iter(apply_gravity);

    rt->world.system<Velocity, Gd::CharacterBody3D>("ApplyVelocity")
        .kind(physics)
        .iter(apply_velocity);

    rt->world.system<Gd::CharacterBody3D>("CheckGrounded")
        .kind(physics)
        .iter(check_grounded);

    // Process phase - visual updates
    rt->world.system<Velocity, Gd::Node3D>("UpdateVisualPosition")
        .kind(process)
        .iter(update_visual_position);
}

}
