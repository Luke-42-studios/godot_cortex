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

static void update_locomotion(flecs::iter& it,
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

static void update_death(flecs::iter& it, Gd::AnimationPlayer* anim) {
    for (auto i : it) {
        if (!anim[i].is_valid()) continue;

        if (anim[i].ptr->get_current_animation() != "death") {
            anim[i].ptr->play("death");
        }
    }
}

// ============================================================================
// Initialization - Single entry point
// ============================================================================
//
// Animation is visual-only, runs in Phase_Process.
// No ordering concerns - just play the right animation.

void init(Runtime* rt) {
    flecs::world& world = rt->world;
    flecs::entity process = rt->phases[Phase_Process].id;

    // Locomotion animation (living entities)
    world.system<Velocity, Gd::AnimationPlayer>("UpdateLocomotion")
        .kind(process)
        .without<Tag::Dead>()
        .iter(update_locomotion);

    // Death animation (dead entities)
    world.system<Gd::AnimationPlayer>("UpdateDeath")
        .kind(process)
        .with<Tag::Dead>()
        .iter(update_death);
}

} // namespace Animation
