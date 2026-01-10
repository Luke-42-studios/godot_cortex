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
// flecs .each() callback signatures:
//   (Components&...)                        - just components
//   (flecs::entity, Components&...)         - need entity access
//   (flecs::iter&, size_t, Components&...)  - need delta_time

static void player_movement(flecs::entity e,
                            const InputState& input,
                            const MoveSpeed& speed,
                            Velocity& vel,
                            Gd::Node3D& node)
{
    if (!node.is_valid()) return;

    // Get camera-relative directions
    godot::Vector3 forward = node.get_forward();
    godot::Vector3 right = forward.cross(godot::Vector3(0, 1, 0));
    forward.y = 0; forward = forward.normalized();
    right.y = 0; right = right.normalized();

    // Apply input to velocity
    godot::Vector3 dir = forward * input.move.y + right * input.move.x;
    vel.linear.x = dir.x * speed.value;
    vel.linear.z = dir.z * speed.value;

    // Handle jump (grounded check via .with<Tag::Grounded>() on separate system)
    if (input.jump && e.has<Tag::Grounded>()) {
        vel.linear.y = 5.0f;
    }
}

// Uses iter signature for delta_time access
static void apply_gravity(flecs::iter& it, size_t i, Velocity& vel) {
    const float gravity = -9.8f;
    vel.linear.y += gravity * it.delta_time();
}

static void apply_velocity(const Velocity& vel, Gd::CharacterBody3D& body) {
    if (!body.is_valid()) return;
    body.set_velocity(vel.linear);
    body.move_and_slide();
}

static void resolve_velocity(Velocity& vel, Gd::CharacterBody3D& body) {
    if (!body.is_valid()) return;
    vel.linear = body.get_velocity();
}

static void check_grounded(flecs::entity e, Gd::CharacterBody3D& body) {
    if (!body.is_valid()) return;

    if (body.is_on_floor()) {
        e.add<Tag::Grounded>();
    } else {
        e.remove<Tag::Grounded>();
    }
}

// ============================================================================
// Initialization - Single entry point
// ============================================================================

void init(Runtime* rt) {
    flecs::world& w = rt->world();
    flecs::entity physics = rt->get_phase(Phase_Physics).id;

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

    Phase::Input = w.entity("Movement.Input")
        .add(flecs::Phase)
        .depends_on(physics);

    Phase::Apply = w.entity("Movement.Apply")
        .add(flecs::Phase)
        .depends_on(Phase::Input);

    Phase::Resolve = w.entity("Movement.Resolve")
        .add(flecs::Phase)
        .depends_on(Phase::Apply);

    // -------------------------------------------------------------------------
    // Register systems with .each() and named functions
    // -------------------------------------------------------------------------

    // Movement.Input - Calculate intent
    w.system<const InputState, const MoveSpeed, Velocity, Gd::Node3D>("PlayerMovement")
        .kind(Phase::Input)
        .with<Tag::Player>()
        .each(player_movement);

    // Movement.Apply - Physics forces (gravity only for airborne)
    w.system<Velocity>("ApplyGravity")
        .kind(Phase::Apply)
        .without<Tag::Grounded>()  // Filter at query level
        .each(apply_gravity);

    w.system<const Velocity, Gd::CharacterBody3D>("ApplyVelocity")
        .kind(Phase::Apply)
        .each(apply_velocity);

    // Movement.Resolve - Read results
    w.system<Velocity, Gd::CharacterBody3D>("ResolveVelocity")
        .kind(Phase::Resolve)
        .each(resolve_velocity);

    w.system<Gd::CharacterBody3D>("CheckGrounded")
        .kind(Phase::Resolve)
        .each(check_grounded);
}

} // namespace Movement
