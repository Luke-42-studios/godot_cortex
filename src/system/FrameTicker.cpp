// =============================================================================
// FrameTicker - Pipeline management implementation
// =============================================================================

#include "FrameTicker.h"
#include "../Log.h"

namespace Polaris {
namespace System {

// Static pipeline registry - one per world (we only have one world currently)
static PipelineRegistry s_registry;

void init_pipelines(flecs::world& world) {
    if (s_registry.initialized) {
        return;  // Already initialized
    }

    Log::info("[Polaris::Pipelines] Initializing custom pipelines...");

    // Create phase entities with the Phase tag
    // These are used with .kind() when registering systems
    s_registry.physics_phase = world.entity("Polaris::PhysicsPhase")
        .add(flecs::Phase)
        .add(flecs::DependsOn, flecs::OnUpdate);

    s_registry.process_phase = world.entity("Polaris::ProcessPhase")
        .add(flecs::Phase)
        .add(flecs::DependsOn, flecs::OnUpdate);

    // Create physics pipeline - matches systems registered to physics_phase
    s_registry.physics_pipeline = world.pipeline()
        .with(flecs::System)
        .with(flecs::DependsOn, s_registry.physics_phase)
        .build();

    // Create process pipeline - matches systems registered to process_phase
    s_registry.process_pipeline = world.pipeline()
        .with(flecs::System)
        .with(flecs::DependsOn, s_registry.process_phase)
        .build();

    s_registry.initialized = true;

    Log::info("[Polaris::Pipelines] Physics and Process pipelines created");
}

flecs::entity get_physics_phase(flecs::world& world) {
    if (!s_registry.initialized) {
        Log::info("[Polaris::Pipelines] WARNING: Pipelines not initialized, call init_pipelines first");
        return flecs::entity();
    }
    return s_registry.physics_phase;
}

flecs::entity get_process_phase(flecs::world& world) {
    if (!s_registry.initialized) {
        Log::info("[Polaris::Pipelines] WARNING: Pipelines not initialized, call init_pipelines first");
        return flecs::entity();
    }
    return s_registry.process_phase;
}

void run_physics_pipeline(flecs::world& world, float delta) {
    if (!s_registry.initialized) {
        return;
    }

    // Run the physics pipeline directly (doesn't affect world time like progress() does)
    world.run_pipeline(s_registry.physics_pipeline, delta);
}

void run_process_pipeline(flecs::world& world, float delta) {
    if (!s_registry.initialized) {
        return;
    }

    // Run the process pipeline directly
    world.run_pipeline(s_registry.process_pipeline, delta);
}

} // namespace System
} // namespace Polaris
