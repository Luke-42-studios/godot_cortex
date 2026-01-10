// examples/domains/animation.cpp
// Animation domain module - handles visual updates
//
// Simple domain: runs in Phase_Process (variable framerate).
// No sub-phases needed - just visual updates.

#include "polaris/core/Runtime.h"
#include "polaris/components/Gd.h"

using namespace Polaris;

namespace Animation {

// ============================================================================
// Systems (static = private to this file)
// ============================================================================
// Simple .each() callbacks with reference parameters

static void update_locomotion(const Velocity& vel, Gd::AnimationPlayer& anim) {
    if (!anim.is_valid()) return;

    float speed = vel.linear.length();
    if (speed > 0.1f) {
        anim.play("walk");
    } else {
        anim.play("idle");
    }
}

static void update_death(Gd::AnimationPlayer& anim) {
    if (!anim.is_valid()) return;

    if (anim.get_current_animation() != "death") {
        anim.play("death");
    }
}

// ============================================================================
// Initialization - Single entry point
// ============================================================================
//
// Animation is visual-only, runs in Phase_Process.
// No ordering concerns - just play the right animation.

void init(Runtime* rt) {
    flecs::world& w = rt->world();
    flecs::entity process = rt->get_phase(Phase_Process).id;

    // Locomotion animation (living entities)
    w.system<const Velocity, Gd::AnimationPlayer>("UpdateLocomotion")
        .kind(process)
        .without<Tag::Dead>()
        .each(update_locomotion);

    // Death animation (dead entities)
    w.system<Gd::AnimationPlayer>("UpdateDeath")
        .kind(process)
        .with<Tag::Dead>()
        .each(update_death);
}

} // namespace Animation
