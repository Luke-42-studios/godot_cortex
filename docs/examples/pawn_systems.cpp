// examples/pawn_systems.cpp
// Complete system implementations for Pawn, PlayerPawn, and AIPawn

#include "polaris/core/Runtime.h"
#include "pawn_composition.cpp"  // Component definitions

namespace Game::Systems {

// ============================================================================
// PHASE: Input - Capture player input
// ============================================================================

void capture_player_input(flecs::iter& it, InputState* input) {
    godot::Input* godot_input = godot::Input::get_singleton();

    for (auto i : it) {
        // Read input actions defined in Godot project settings
        input[i].move = godot_input->get_vector(
            "move_left", "move_right", "move_forward", "move_back"
        );
        input[i].look = godot_input->get_last_mouse_velocity() * 0.001f;
        input[i].jump = godot_input->is_action_just_pressed("jump");
        input[i].attack = godot_input->is_action_just_pressed("attack");
    }
}

// ============================================================================
// PHASE: Physics - Game logic at fixed timestep
// ============================================================================

// --- Movement Systems ---

void player_movement(flecs::iter& it,
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

void ai_movement(flecs::iter& it,
                 AI* ai,
                 MoveSpeed* speed,
                 Velocity* vel,
                 Gd::Node3D* node)
{
    for (auto i : it) {
        if (!node[i].is_valid()) continue;

        switch (ai[i].state) {
            case AIState::Idle:
                // Stand still
                vel[i].linear.x = 0;
                vel[i].linear.z = 0;
                break;

            case AIState::Patrol:
                // Simple patrol pattern
                vel[i].linear.x = speed[i].value * 0.5f;
                vel[i].linear.z = 0;
                break;

            case AIState::Chase:
                if (ai[i].target.is_alive()) {
                    // Move toward target
                    auto* target_node = ai[i].target.get<Gd::Node3D>();
                    if (target_node && target_node->is_valid()) {
                        godot::Vector3 dir = target_node->get_position() - node[i].get_position();
                        dir.y = 0;
                        dir = dir.normalized();
                        vel[i].linear.x = dir.x * speed[i].value;
                        vel[i].linear.z = dir.z * speed[i].value;
                    }
                }
                break;

            case AIState::Attack:
                // Stop moving to attack
                vel[i].linear.x = 0;
                vel[i].linear.z = 0;
                break;
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

// --- AI Systems ---

void ai_state_machine(flecs::iter& it, AI* ai, Gd::Node3D* node) {
    // Use phase arena for temporary allocations
    Arena* arena = &runtime()->phases[Phase_Physics].arena;
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
                    // Check if close enough to attack
                    auto* target_node = ai[i].target.get<Gd::Node3D>();
                    if (target_node && target_node->is_valid()) {
                        float dist = node[i].get_position().distance_to(target_node->get_position());
                        if (dist < 2.0f) {
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

void ai_target_acquisition(flecs::iter& it, AI* ai, Gd::Node3D* node) {
    for (auto i : it) {
        if (!node[i].is_valid()) continue;
        if (ai[i].state == AIState::Chase || ai[i].state == AIState::Attack) continue;

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

// --- Combat Systems ---

void check_death(flecs::iter& it, Health* hp) {
    for (auto i : it) {
        if (hp[i].current <= 0) {
            flecs::entity e = it.entity(i);
            e.add<Tag::Dead>();
        }
    }
}

// ============================================================================
// PHASE: Process - Visual updates at variable framerate
// ============================================================================

void update_animation(flecs::iter& it,
                      Velocity* vel,
                      Gd::AnimationPlayer* anim)
{
    for (auto i : it) {
        if (!anim[i].is_valid()) continue;

        float speed = vel[i].linear.length();
        if (speed > 0.1f) {
            anim[i].ptr->play("walk");
        } else {
            anim[i].ptr->play("idle");
        }
    }
}

void update_death_animation(flecs::iter& it, Gd::AnimationPlayer* anim) {
    for (auto i : it) {
        if (!anim[i].is_valid()) continue;

        if (anim[i].ptr->get_current_animation() != "death") {
            anim[i].ptr->play("death");
        }
    }
}

// ============================================================================
// REGISTRATION
// ============================================================================

void register_pawn_systems(Runtime* rt) {
    flecs::entity input = rt->phases[Phase_Input].id;
    flecs::entity physics = rt->phases[Phase_Physics].id;
    flecs::entity process = rt->phases[Phase_Process].id;

    // --- Input Phase ---
    rt->world.system<InputState>("CapturePlayerInput")
        .kind(input)
        .with<Tag::Player>()
        .iter(capture_player_input);

    // --- Physics Phase (order matters) ---
    
    // 1. Input processing
    rt->world.system<InputState, MoveSpeed, Velocity, Gd::Node3D>
        ("PlayerMovement")
        .kind(physics)
        .with<Tag::Player>()
        .iter(player_movement);

    rt->world.system<AI, MoveSpeed, Velocity, Gd::Node3D>
        ("AIMovement")
        .kind(physics)
        .with<Tag::Enemy>()
        .without<Tag::Dead>()
        .iter(ai_movement);

    // 2. Physics simulation
    rt->world.system<Velocity>("ApplyGravity")
        .kind(physics)
        .iter(apply_gravity);

    rt->world.system<Velocity, Gd::CharacterBody3D>("ApplyVelocity")
        .kind(physics)
        .iter(apply_velocity);

    rt->world.system<Gd::CharacterBody3D>("CheckGrounded")
        .kind(physics)
        .iter(check_grounded);

    // 3. AI logic
    rt->world.system<AI, Gd::Node3D>("AITargetAcquisition")
        .kind(physics)
        .with<Tag::Enemy>()
        .without<Tag::Dead>()
        .iter(ai_target_acquisition);

    rt->world.system<AI, Gd::Node3D>("AIStateMachine")
        .kind(physics)
        .with<Tag::Enemy>()
        .without<Tag::Dead>()
        .iter(ai_state_machine);

    // 4. Combat
    rt->world.system<Health>("CheckDeath")
        .kind(physics)
        .without<Tag::Dead>()
        .iter(check_death);

    // --- Process Phase ---
    rt->world.system<Velocity, Gd::AnimationPlayer>("UpdateAnimation")
        .kind(process)
        .without<Tag::Dead>()
        .iter(update_animation);

    rt->world.system<Gd::AnimationPlayer>("UpdateDeathAnimation")
        .kind(process)
        .with<Tag::Dead>()
        .iter(update_death_animation);
}

}
