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

// --- Sense Phase ---

static void target_acquisition(flecs::iter& it, AIComponent* ai, Gd::Node3D* node) {
    for (auto i : it) {
        if (!node[i].is_valid()) continue;

        // Already has a valid target
        if (ai[i].target.is_alive()) continue;

        godot::Vector3 pos = node[i].get_position();

        // Find nearest player within aggro range
        runtime()->world.each<Tag::Player, Gd::Node3D>(
            [&](flecs::entity player, Tag::Player, Gd::Node3D& player_node) {
                if (!player_node.is_valid()) return;

                float dist = pos.distance_to(player_node.get_position());
                if (dist < ai[i].aggro_range) {
                    ai[i].target = player;
                    ai[i].state = AIState::Chase;
                }
            }
        );
    }
}

// --- Decide Phase ---

static void state_machine(flecs::iter& it, AIComponent* ai, Gd::Node3D* node) {
    float dt = it.delta_time();

    for (auto i : it) {
        if (!node[i].is_valid()) continue;

        ai[i].timer -= dt;

        switch (ai[i].state) {
            case AIState::Idle:
                if (ai[i].timer <= 0) {
                    ai[i].state = AIState::Patrol;
                    ai[i].timer = 5.0f;
                }
                break;

            case AIState::Patrol:
                if (ai[i].timer <= 0) {
                    ai[i].state = AIState::Idle;
                    ai[i].timer = 2.0f;
                }
                break;

            case AIState::Chase:
                if (!ai[i].target.is_alive()) {
                    ai[i].state = AIState::Idle;
                    ai[i].timer = 1.0f;
                } else {
                    // Check distance to target
                    auto* target_node = ai[i].target.get<Gd::Node3D>();
                    if (target_node && target_node->is_valid()) {
                        float dist = node[i].get_position().distance_to(
                            target_node->get_position()
                        );
                        if (dist < ai[i].attack_range) {
                            ai[i].state = AIState::Attack;
                            ai[i].timer = 1.0f;
                        }
                    }
                }
                break;

            case AIState::Attack:
                if (ai[i].timer <= 0) {
                    ai[i].state = AIState::Chase;
                    ai[i].timer = 0.5f;
                }
                break;
        }
    }
}

// --- Act Phase ---

static void ai_movement(flecs::iter& it,
                        AIComponent* ai,
                        MoveSpeed* speed,
                        Velocity* vel,
                        Gd::Node3D* node)
{
    for (auto i : it) {
        if (!node[i].is_valid()) continue;

        switch (ai[i].state) {
            case AIState::Idle:
                vel[i].linear.x = 0;
                vel[i].linear.z = 0;
                break;

            case AIState::Patrol:
                vel[i].linear.x = speed[i].value * 0.5f;
                vel[i].linear.z = 0;
                break;

            case AIState::Chase:
                if (ai[i].target.is_alive()) {
                    auto* target_node = ai[i].target.get<Gd::Node3D>();
                    if (target_node && target_node->is_valid()) {
                        godot::Vector3 dir = target_node->get_position() -
                                             node[i].get_position();
                        dir.y = 0;
                        dir = dir.normalized();
                        vel[i].linear.x = dir.x * speed[i].value;
                        vel[i].linear.z = dir.z * speed[i].value;
                    }
                }
                break;

            case AIState::Attack:
                vel[i].linear.x = 0;
                vel[i].linear.z = 0;
                break;
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

    Phase::Sense = world.entity("AI.Sense")
        .add(flecs::Phase)
        .depends_on(physics);

    Phase::Decide = world.entity("AI.Decide")
        .add(flecs::Phase)
        .depends_on(Phase::Sense);

    Phase::Act = world.entity("AI.Act")
        .add(flecs::Phase)
        .depends_on(Phase::Decide);

    // -------------------------------------------------------------------------
    // Register systems to sub-phases
    // -------------------------------------------------------------------------

    world.system<AIComponent, Gd::Node3D>("AITargetAcquisition")
        .kind(Phase::Sense)
        .with<Tag::Enemy>()
        .without<Tag::Dead>()
        .iter(target_acquisition);

    world.system<AIComponent, Gd::Node3D>("AIStateMachine")
        .kind(Phase::Decide)
        .with<Tag::Enemy>()
        .without<Tag::Dead>()
        .iter(state_machine);

    world.system<AIComponent, MoveSpeed, Velocity, Gd::Node3D>("AIMovement")
        .kind(Phase::Act)
        .with<Tag::Enemy>()
        .without<Tag::Dead>()
        .iter(ai_movement);
}

} // namespace AI
