// examples/domains/movement.cpp
// Movement domain module - handles all locomotion
//
// This module demonstrates the Domain Module pattern:
// - Self-contained initialization
// - Local sub-phases for ordering
// - Static system functions (private to file)
//
// NAMESPACE CONVENTION:
//   Polaris::     - Engine code (Runtime, Gd::, Tag::)
//   Game::        - Your game code (components, domains)
//
// In a real project, components would be in Game:: namespace in separate headers.
// Here they're defined locally for demonstration.

#include "polaris/core/Runtime.h"
#include "polaris/components/Gd.h"

using namespace Polaris;  // Access Gd::, Tag::, runtime(), etc.

// In production: namespace Game::Movement {
namespace Movement {

// ============================================================================
// Sub-Phases
// ============================================================================
// Movement needs precise ordering: calculate intent -> apply forces -> resolve
// Sub-phases give explicit control via DependsOn, not registration order.

namespace Phase {
    flecs::entity Input;    // Calculate movement direction from input
    flecs::entity Apply;    // Apply gravity, velocity to physics body
    flecs::entity Resolve;  // Read collision results, update state
}

// ============================================================================
// Components (defined locally for example - normally in Game:: namespace)
// ============================================================================

struct Velocity {
    godot::Vector3 linear;
};

struct MoveSpeed {
    float value = 5.0f;
};

struct InputState {
    godot::Vector2 move;
    bool jump;
};

// ============================================================================
// Systems (static = private to this file)
// ============================================================================

static void player_movement(flecs::iter& it,
                            InputState* input,
                            MoveSpeed* speed,
                            Velocity* vel,
                            Gd::Node3D* node)
{
    for (auto i : it) {
        if (!node[i].is_valid()) continue;

        // Get camera-relative directions
        godot::Vector3 forward = node[i].get_forward();
        godot::Vector3 right = forward.cross(godot::Vector3(0, 1, 0));
        forward.y = 0; forward = forward.normalized();
        right.y = 0; right = right.normalized();

        // Apply input to velocity
        godot::Vector3 dir = forward * input[i].move.y + right * input[i].move.x;
        vel[i].linear.x = dir.x * speed[i].value;
        vel[i].linear.z = dir.z * speed[i].value;

        // Handle jump
        if (input[i].jump && it.entity(i).has<Tag::Grounded>()) {
            vel[i].linear.y = 5.0f;
        }
    }
}

static void apply_gravity(flecs::iter& it, Velocity* vel) {
    const float gravity = -9.8f;
    float dt = it.delta_time();

    for (auto i : it) {
        if (!it.entity(i).has<Tag::Grounded>()) {
            vel[i].linear.y += gravity * dt;
        }
    }
}

static void apply_velocity(flecs::iter& it,
                           Velocity* vel,
                           Gd::CharacterBody3D* body)
{
    for (auto i : it) {
        if (!body[i].is_valid()) continue;

        body[i].ptr->set_velocity(vel[i].linear);
        body[i].ptr->move_and_slide();
    }
}

static void resolve_velocity(flecs::iter& it,
                             Velocity* vel,
                             Gd::CharacterBody3D* body)
{
    for (auto i : it) {
        if (!body[i].is_valid()) continue;

        // Read back actual velocity after collision
        vel[i].linear = body[i].ptr->get_velocity();
    }
}

static void check_grounded(flecs::iter& it, Gd::CharacterBody3D* body) {
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
// Initialization - Single entry point
// ============================================================================

void init(Runtime* rt) {
    flecs::world& world = rt->world;
    flecs::entity physics = rt->phases[Phase_Physics].id;

    // -------------------------------------------------------------------------
    // Create sub-phases with explicit dependencies
    // -------------------------------------------------------------------------
    //
    //   Physics (parent)
    //       |
    //       v
    //   Movement.Input  <-- Calculate direction from player input
    //       |
    //       v
    //   Movement.Apply  <-- Apply gravity, move_and_slide()
    //       |
    //       v
    //   Movement.Resolve <-- Read collision results, check grounded
    //

    Phase::Input = world.entity("Movement.Input")
        .add(flecs::Phase)
        .depends_on(physics);

    Phase::Apply = world.entity("Movement.Apply")
        .add(flecs::Phase)
        .depends_on(Phase::Input);

    Phase::Resolve = world.entity("Movement.Resolve")
        .add(flecs::Phase)
        .depends_on(Phase::Apply);

    // -------------------------------------------------------------------------
    // Register systems to sub-phases
    // -------------------------------------------------------------------------

    // Movement.Input - Calculate intent
    world.system<InputState, MoveSpeed, Velocity, Gd::Node3D>("PlayerMovement")
        .kind(Phase::Input)
        .with<Tag::Player>()
        .iter(player_movement);

    // Movement.Apply - Physics forces
    world.system<Velocity>("ApplyGravity")
        .kind(Phase::Apply)
        .iter(apply_gravity);

    world.system<Velocity, Gd::CharacterBody3D>("ApplyVelocity")
        .kind(Phase::Apply)
        .iter(apply_velocity);

    // Movement.Resolve - Read results
    world.system<Velocity, Gd::CharacterBody3D>("ResolveVelocity")
        .kind(Phase::Resolve)
        .iter(resolve_velocity);

    world.system<Gd::CharacterBody3D>("CheckGrounded")
        .kind(Phase::Resolve)
        .iter(check_grounded);
}

} // namespace Movement
