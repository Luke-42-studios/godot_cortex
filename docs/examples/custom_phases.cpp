// examples/custom_phases.cpp
// Shows how to create custom sub-phases for fine-grained control

#include "polaris/core/Runtime.h"

namespace Game {

// ============================================================================
// Custom Phase Structure
// ============================================================================

struct CustomPhases {
    // Physics sub-phases for precise ordering
    struct {
        flecs::entity input;        // Read input
        flecs::entity movement;     // Calculate movement
        flecs::entity apply;        // Apply to physics bodies
        flecs::entity collision;    // Handle collision responses
    } physics;

    // Process sub-phases
    struct {
        flecs::entity interpolate;  // Interpolate positions
        flecs::entity animate;      // Update animations
        flecs::entity effects;      // Spawn particles, sounds
    } process;
};

// ============================================================================
// Setup Custom Phases
// ============================================================================

CustomPhases setup_custom_phases(Runtime* rt) {
    CustomPhases phases;

    // Create physics sub-phases as children of Phase_Physics
    flecs::entity physics_root = rt->phases[Phase_Physics].id;

    phases.physics.input = rt->world.entity("Physics.Input")
        .add(flecs::ChildOf, physics_root);

    phases.physics.movement = rt->world.entity("Physics.Movement")
        .add(flecs::ChildOf, physics_root)
        .add(flecs::DependsOn, phases.physics.input);

    phases.physics.apply = rt->world.entity("Physics.Apply")
        .add(flecs::ChildOf, physics_root)
        .add(flecs::DependsOn, phases.physics.movement);

    phases.physics.collision = rt->world.entity("Physics.Collision")
        .add(flecs::ChildOf, physics_root)
        .add(flecs::DependsOn, phases.physics.apply);

    // Create process sub-phases
    flecs::entity process_root = rt->phases[Phase_Process].id;

    phases.process.interpolate = rt->world.entity("Process.Interpolate")
        .add(flecs::ChildOf, process_root);

    phases.process.animate = rt->world.entity("Process.Animate")
        .add(flecs::ChildOf, process_root)
        .add(flecs::DependsOn, phases.process.interpolate);

    phases.process.effects = rt->world.entity("Process.Effects")
        .add(flecs::ChildOf, process_root)
        .add(flecs::DependsOn, phases.process.animate);

    return phases;
}

// ============================================================================
// Using Custom Phases
// ============================================================================

void register_systems_with_custom_phases(Runtime* rt, const CustomPhases& phases) {
    
    // Register to specific sub-phases for precise control
    rt->world.system<InputState>("CaptureInput")
        .kind(phases.physics.input)
        .iter([](flecs::iter& it, InputState* input) {
            // Capture input first
        });

    rt->world.system<Velocity, MoveSpeed>("CalculateMovement")
        .kind(phases.physics.movement)
        .iter([](flecs::iter& it, Velocity* vel, MoveSpeed* speed) {
            // Calculate desired movement
        });

    rt->world.system<Velocity, Gd::CharacterBody3D>("MoveAndSlide")
        .kind(phases.physics.apply)
        .iter([](flecs::iter& it, Velocity* vel, Gd::CharacterBody3D* body) {
            // Apply movement to physics bodies
            for (auto i : it) {
                if (!body[i].is_valid()) continue;
                
                body[i].ptr->set_velocity(vel[i].linear);
                body[i].ptr->move_and_slide();
                vel[i].linear = body[i].ptr->get_velocity();
            }
        });

    rt->world.system<Gd::CharacterBody3D>("HandleCollisions")
        .kind(phases.physics.collision)
        .iter([](flecs::iter& it, Gd::CharacterBody3D* body) {
            // React to collisions after all movement is applied
        });

    // Process phase sub-phases
    rt->world.system<Transform, Velocity>("InterpolatePosition")
        .kind(phases.process.interpolate)
        .iter([](flecs::iter& it, Transform* xform, Velocity* vel) {
            // Smooth interpolation
        });

    rt->world.system<Gd::AnimationPlayer, Velocity>("UpdateAnimation")
        .kind(phases.process.animate)
        .iter([](flecs::iter& it, Gd::AnimationPlayer* anim, Velocity* vel) {
            // Update animations based on interpolated state
        });
}

// ============================================================================
// Alternative: Explicit Dependencies Without Sub-Phases
// ============================================================================

void register_with_explicit_dependencies(Runtime* rt) {
    flecs::entity physics = rt->phases[Phase_Physics].id;

    // Systems run in registration order by default
    // Just register in the order you want them to execute:
    
    auto capture = rt->world.system<InputState>("CaptureInput")
        .kind(physics)
        .iter([](flecs::iter& it, InputState* input) { /* ... */ });

    auto calculate = rt->world.system<Velocity>("CalculateMovement")
        .kind(physics)
        .iter([](flecs::iter& it, Velocity* vel) { /* ... */ });

    auto apply = rt->world.system<Velocity, Gd::CharacterBody3D>("ApplyMovement")
        .kind(physics)
        .iter([](flecs::iter& it, Velocity* vel, Gd::CharacterBody3D* body) { /* ... */ });

    // Or use explicit dependencies if you need to register out-of-order:
    rt->world.system<Gd::CharacterBody3D>("HandleCollisions")
        .kind(physics)
        .add(flecs::DependsOn, apply)  // Explicitly runs after ApplyMovement
        .iter([](flecs::iter& it, Gd::CharacterBody3D* body) { /* ... */ });
}

}
