#ifndef POLARIS_SYSTEM_FRAME_TICKER_H
#define POLARIS_SYSTEM_FRAME_TICKER_H

#include <cstdint>

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

/// Current phase indicator - systems can check this to know which phase is active
enum class FramePhase : uint8_t {
    None = 0,
    Physics = 1,
    Process = 2
};

struct CurrentPhase {
    FramePhase phase = FramePhase::None;
};

// =============================================================================
// Custom Flecs Phases
// Systems register to these phases to run at the appropriate time.
// =============================================================================

/// Tag for physics phase - systems here run during physics_frame
struct OnPhysicsPhase {};

/// Tag for process phase - systems here run during process_frame
struct OnProcessPhase {};

} // namespace System
} // namespace Polaris

#endif // POLARIS_SYSTEM_FRAME_TICKER_H
