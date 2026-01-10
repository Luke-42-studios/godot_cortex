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
// flecs .each() callback signatures:
//   (Components&...)                        - just components
//   (flecs::entity, Components&...)         - need entity access
//   (flecs::iter&, size_t, Components&...)  - need delta_time

static void apply_damage(Health& hp, DamageQueue& dmg) {
    if (dmg.pending > 0) {
        hp.current -= dmg.pending;
        dmg.pending = 0;  // Clear queue
    }
}

static void check_death(flecs::entity e, Health& hp) {
    if (hp.current <= 0) {
        e.add<Tag::Dead>();
    }
}

// Uses iter signature for delta_time access
static void health_regen(flecs::iter& it, size_t i, Health& hp) {
    const float regen_rate = 1.0f;  // HP per second
    if (hp.current < hp.max) {
        hp.current = fminf(hp.current + regen_rate * it.delta_time(), hp.max);
    }
}

// ============================================================================
// Initialization - Single entry point
// ============================================================================
//
// Combat is simple: no sub-phases needed.
// Order is handled by registration within this function.

void init(Runtime* rt) {
    flecs::world& w = rt->world();
    flecs::entity physics = rt->get_phase(Phase_Physics).id;

    // Systems run in registration order within the phase
    w.system<Health, DamageQueue>("ApplyDamage")
        .kind(physics)
        .each(apply_damage);

    w.system<Health>("CheckDeath")
        .kind(physics)
        .without<Tag::Dead>()
        .each(check_death);

    w.system<Health>("HealthRegen")
        .kind(physics)
        .without<Tag::Dead>()
        .each(health_regen);
}

} // namespace Combat
