// examples/domains/combat.cpp
// Combat domain module - handles damage and death
//
// Simple domain example: no sub-phases needed.
// Systems register directly to Phase_Physics.

#include "polaris/core/Runtime.h"
#include "polaris/components/Gd.h"

using namespace Polaris;

namespace Combat {

// ============================================================================
// Components
// ============================================================================

struct Health {
    float current = 100.0f;
    float max = 100.0f;
};

struct DamageQueue {
    float pending = 0.0f;  // Accumulated damage this frame
};

// ============================================================================
// Systems (static = private to this file)
// ============================================================================

static void apply_damage(flecs::iter& it, Health* hp, DamageQueue* dmg) {
    for (auto i : it) {
        if (dmg[i].pending > 0) {
            hp[i].current -= dmg[i].pending;
            dmg[i].pending = 0;  // Clear queue
        }
    }
}

static void check_death(flecs::iter& it, Health* hp) {
    for (auto i : it) {
        if (hp[i].current <= 0) {
            it.entity(i).add<Tag::Dead>();
        }
    }
}

static void health_regen(flecs::iter& it, Health* hp) {
    float dt = it.delta_time();
    const float regen_rate = 1.0f;  // HP per second

    for (auto i : it) {
        if (hp[i].current < hp[i].max) {
            hp[i].current = fminf(hp[i].current + regen_rate * dt, hp[i].max);
        }
    }
}

// ============================================================================
// Initialization - Single entry point
// ============================================================================
//
// Combat is simple: no sub-phases needed.
// Order is handled by registration within this function.

void init(Runtime* rt) {
    flecs::world& world = rt->world;
    flecs::entity physics = rt->phases[Phase_Physics].id;

    // Systems run in registration order within the phase
    world.system<Health, DamageQueue>("ApplyDamage")
        .kind(physics)
        .iter(apply_damage);

    world.system<Health>("CheckDeath")
        .kind(physics)
        .without<Tag::Dead>()
        .iter(check_death);

    world.system<Health>("HealthRegen")
        .kind(physics)
        .without<Tag::Dead>()
        .iter(health_regen);
}

} // namespace Combat
