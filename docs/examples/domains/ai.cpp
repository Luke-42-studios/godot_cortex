// examples/domains/ai.cpp
// AI domain module - handles enemy behavior
//
// Complex domain with sub-phases: Sense -> Decide -> Act
// This ensures AI sees updated world state before making decisions.

#include "polaris/core/Runtime.h"
#include "polaris/components/Gd.h"

using namespace Polaris;

namespace AI {

// ============================================================================
// Sub-Phases
// ============================================================================
// AI needs clear data flow: perceive world -> make decision -> execute action
// Sub-phases ensure each step sees results of previous step.

namespace Phase {
    flecs::entity Sense;   // Detect targets, read world state
    flecs::entity Decide;  // State machine, pathfinding
    flecs::entity Act;     // Apply decisions to velocity
}

// ============================================================================
// Components
// ============================================================================

enum class AIState {
    Idle,
    Patrol,
    Chase,
    Attack
};

struct AIComponent {
    AIState state = AIState::Idle;
    flecs::entity target;
    float timer = 0;
    float aggro_range = 10.0f;
    float attack_range = 2.0f;
};

// ============================================================================
// Systems (static = private to this file)
// ============================================================================
// flecs .each() callback signatures:
//   (Components&...)                        - just components
//   (flecs::entity, Components&...)         - need entity access
//   (flecs::iter&, size_t, Components&...)  - need delta_time

// --- Sense Phase ---

static void target_acquisition(AIComponent& ai, Gd::Node3D& node) {
    if (!node.is_valid()) return;

    // Already has a valid target
    if (ai.target.is_alive()) return;

    godot::Vector3 pos = node.get_position();

    // Find nearest player within aggro range
    runtime()->world().each<Tag::Player, Gd::Node3D>(
        [&](flecs::entity player, Tag::Player, Gd::Node3D& player_node) {
            if (!player_node.is_valid()) return;

            float dist = pos.distance_to(player_node.get_position());
            if (dist < ai.aggro_range) {
                ai.target = player;
                ai.state = AIState::Chase;
            }
        }
    );
}

// --- Decide Phase ---
// Uses iter signature for delta_time access

static void state_machine(flecs::iter& it, size_t i, AIComponent& ai, Gd::Node3D& node) {
    if (!node.is_valid()) return;

    float dt = it.delta_time();
    ai.timer -= dt;

    switch (ai.state) {
        case AIState::Idle:
            if (ai.timer <= 0) {
                ai.state = AIState::Patrol;
                ai.timer = 5.0f;
            }
            break;

        case AIState::Patrol:
            if (ai.timer <= 0) {
                ai.state = AIState::Idle;
                ai.timer = 2.0f;
            }
            break;

        case AIState::Chase:
            if (!ai.target.is_alive()) {
                ai.state = AIState::Idle;
                ai.timer = 1.0f;
            } else {
                // Check distance to target
                auto* target_node = ai.target.get<Gd::Node3D>();
                if (target_node && target_node->is_valid()) {
                    float dist = node.get_position().distance_to(
                        target_node->get_position()
                    );
                    if (dist < ai.attack_range) {
                        ai.state = AIState::Attack;
                        ai.timer = 1.0f;
                    }
                }
            }
            break;

        case AIState::Attack:
            if (ai.timer <= 0) {
                ai.state = AIState::Chase;
                ai.timer = 0.5f;
            }
            break;
    }
}

// --- Act Phase ---

static void ai_movement(const AIComponent& ai,
                        const MoveSpeed& speed,
                        Velocity& vel,
                        Gd::Node3D& node)
{
    if (!node.is_valid()) return;

    switch (ai.state) {
        case AIState::Idle:
            vel.linear.x = 0;
            vel.linear.z = 0;
            break;

        case AIState::Patrol:
            vel.linear.x = speed.value * 0.5f;
            vel.linear.z = 0;
            break;

        case AIState::Chase:
            if (ai.target.is_alive()) {
                auto* target_node = ai.target.get<Gd::Node3D>();
                if (target_node && target_node->is_valid()) {
                    godot::Vector3 dir = target_node->get_position() -
                                         node.get_position();
                    dir.y = 0;
                    dir = dir.normalized();
                    vel.linear.x = dir.x * speed.value;
                    vel.linear.z = dir.z * speed.value;
                }
            }
            break;

        case AIState::Attack:
            vel.linear.x = 0;
            vel.linear.z = 0;
            break;
    }
}

// ============================================================================
// Initialization - Single entry point
// ============================================================================

void init(Runtime* rt) {
    flecs::world& w = rt->world();
    flecs::entity physics = rt->get_phase(Phase_Physics).id;

    // -------------------------------------------------------------------------
    // Create sub-phases: Sense -> Decide -> Act
    // -------------------------------------------------------------------------
    //
    //   Physics (parent)
    //       |
    //       v
    //   AI.Sense  <-- Detect targets, read positions
    //       |
    //       v
    //   AI.Decide <-- State machine transitions
    //       |
    //       v
    //   AI.Act    <-- Apply velocity based on state
    //

    Phase::Sense = w.entity("AI.Sense")
        .add(flecs::Phase)
        .depends_on(physics);

    Phase::Decide = w.entity("AI.Decide")
        .add(flecs::Phase)
        .depends_on(Phase::Sense);

    Phase::Act = w.entity("AI.Act")
        .add(flecs::Phase)
        .depends_on(Phase::Decide);

    // -------------------------------------------------------------------------
    // Register systems with .each() and named functions
    // -------------------------------------------------------------------------

    w.system<AIComponent, Gd::Node3D>("AITargetAcquisition")
        .kind(Phase::Sense)
        .with<Tag::Enemy>()
        .without<Tag::Dead>()
        .each(target_acquisition);

    w.system<AIComponent, Gd::Node3D>("AIStateMachine")
        .kind(Phase::Decide)
        .with<Tag::Enemy>()
        .without<Tag::Dead>()
        .each(state_machine);

    w.system<const AIComponent, const MoveSpeed, Velocity, Gd::Node3D>("AIMovement")
        .kind(Phase::Act)
        .with<Tag::Enemy>()
        .without<Tag::Dead>()
        .each(ai_movement);
}

} // namespace AI
