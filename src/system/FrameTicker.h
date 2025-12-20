#ifndef POLARIS_SYSTEM_FRAME_TICKER_H
#define POLARIS_SYSTEM_FRAME_TICKER_H

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <flecs.h>
#include <mutex>

#include "../Log.h"

namespace Polaris {
namespace System {

using namespace godot;

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

// =============================================================================
// FrameTicker - Bridges Godot frame events to ECS
//
// Connects to SceneTree's physics_frame and process_frame signals.
// Each frame:
//   1. Updates the frame singleton component (PhysicsFrame or ProcessFrame)
//   2. Calls world.progress() to run all registered systems
//
// Systems should register with appropriate phases:
//   world.system<Movement>("PlayerMove")
//       .kind<OnPhysicsPhase>()
//       .each(...);
// =============================================================================
class FrameTicker : public Object {
    GDCLASS(FrameTicker, Object)

private:
    static inline FrameTicker* singleton_instance = nullptr;

    SceneTree* m_scene_tree = nullptr;
    flecs::world* m_world = nullptr;

    bool m_debug_enabled = false;
    bool m_physics_enabled = true;
    bool m_process_enabled = true;

    // Frame counters
    uint64_t m_physics_frame = 0;
    uint64_t m_process_frame = 0;
    double m_physics_time = 0.0;
    double m_process_time = 0.0;

    // Flecs phase entities
    flecs::entity m_physics_phase;
    flecs::entity m_process_phase;

    FrameTicker(const FrameTicker&) = delete;

protected:
    static void _bind_methods();

public:
    FrameTicker();
    ~FrameTicker();

    // =========================================================================
    // Configuration
    // =========================================================================

    void set_debug_enabled(bool enabled) { m_debug_enabled = enabled; }
    bool get_debug_enabled() const { return m_debug_enabled; }

    void set_physics_enabled(bool enabled) { m_physics_enabled = enabled; }
    bool get_physics_enabled() const { return m_physics_enabled; }

    void set_process_enabled(bool enabled) { m_process_enabled = enabled; }
    bool get_process_enabled() const { return m_process_enabled; }

    // =========================================================================
    // Initialization
    // =========================================================================

    void initialize(flecs::world* world);
    void bind_to_scene_tree(SceneTree* tree);
    void unbind();

    [[nodiscard]] bool is_bound() const noexcept { return m_scene_tree != nullptr; }

    // =========================================================================
    // Phase Access (for system registration)
    // =========================================================================

    [[nodiscard]] flecs::entity get_physics_phase() const { return m_physics_phase; }
    [[nodiscard]] flecs::entity get_process_phase() const { return m_process_phase; }

    // =========================================================================
    // Frame Data Access
    // =========================================================================

    [[nodiscard]] uint64_t get_physics_frame_count() const { return m_physics_frame; }
    [[nodiscard]] uint64_t get_process_frame_count() const { return m_process_frame; }
    [[nodiscard]] double get_physics_time() const { return m_physics_time; }
    [[nodiscard]] double get_process_time() const { return m_process_time; }

    // =========================================================================
    // Singleton
    // =========================================================================

    static FrameTicker* get_singleton() noexcept { return singleton_instance; }
    static FrameTicker* create_global_instance();
    static void destroy_global_instance();

private:
    void _on_physics_frame();
    void _on_process_frame();
    void _setup_phases();
};

} // namespace System
} // namespace Polaris

#endif // POLARIS_SYSTEM_FRAME_TICKER_H
