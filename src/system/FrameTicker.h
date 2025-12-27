#ifndef POLARIS_SYSTEM_FRAME_TICKER_H
#define POLARIS_SYSTEM_FRAME_TICKER_H

#include <cstdint>
#include <flecs.h>

namespace Polaris {
namespace System {

// =============================================================================
// Frame Singleton Components
// These are set each frame before systems run, allowing systems to access
// frame timing information.
// =============================================================================

/// Physics frame data - updated every physics tick
struct PhysicsFrame {
    double delta = 0.0;         // Time since last physics frame
    uint64_t frame = 0;         // Physics frame counter
    double time = 0.0;          // Total elapsed physics time
};

/// Process frame data - updated every render frame
struct ProcessFrame {
    double delta = 0.0;         // Time since last process frame
    uint64_t frame = 0;         // Process frame counter
    double time = 0.0;          // Total elapsed process time
};

/// Current phase indicator (kept for backwards compatibility)
enum class FramePhase : uint8_t {
    None = 0,
    Physics = 1,
    Process = 2
};

struct CurrentPhase {
    FramePhase phase = FramePhase::None;
};

/// Input accumulator - stores mouse motion between frames
/// Systems should read and clear this each physics frame
struct InputAccumulator {
    float mouse_delta_x = 0.0f;
    float mouse_delta_y = 0.0f;
};

// =============================================================================
// Pipeline Registry
// Manages custom Flecs pipelines for different game phases.
// Systems register to a specific pipeline and only run during that phase.
//
// Usage:
//   // Register a physics system (runs during _physics_process)
//   world.system<Position, Velocity>("Move")
//       .kind(Polaris::System::get_physics_phase(world))
//       .each([](Position& p, Velocity& v) { ... });
//
//   // Register a process system (runs during _process)
//   world.system<Transform>("Interpolate")
//       .kind(Polaris::System::get_process_phase(world))
//       .each([](Transform& t) { ... });
// =============================================================================

/// Pipeline registry singleton - stores phase and pipeline entities
struct PipelineRegistry {
    // Phase entities (used with .kind() when registering systems)
    flecs::entity physics_phase;
    flecs::entity process_phase;

    // Pipeline entities (run during appropriate callbacks)
    flecs::entity physics_pipeline;
    flecs::entity process_pipeline;

    bool initialized = false;
};

/// Initialize the pipeline registry - called by TickerNode
void init_pipelines(flecs::world& world);

/// Get the physics phase entity for system registration
flecs::entity get_physics_phase(flecs::world& world);

/// Get the process phase entity for system registration
flecs::entity get_process_phase(flecs::world& world);

/// Run the physics pipeline (called by TickerNode during _physics_process)
void run_physics_pipeline(flecs::world& world, float delta);

/// Run the process pipeline (called by TickerNode during _process)
void run_process_pipeline(flecs::world& world, float delta);

} // namespace System
} // namespace Polaris

#endif // POLARIS_SYSTEM_FRAME_TICKER_H
