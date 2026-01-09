#ifndef POLARIS_RUNTIME_H
#define POLARIS_RUNTIME_H

#include "memory/Arena.h"
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <flecs.h>
#include <cstdint>

namespace Polaris {

using namespace godot;

// =============================================================================
// Phase Types
// =============================================================================

enum PhaseType {
    Phase_Input,      // _input() - event driven
    Phase_Physics,    // _physics_process() - fixed timestep (60hz)
    Phase_Process,    // _process() - variable timestep
    Phase_Count
};

// =============================================================================
// Phase - Execution bucket with dedicated arena
// =============================================================================

struct Phase {
    Arena arena;        // Temporary memory for this phase (resets after phase)
    flecs::entity id;   // Flecs entity for system ordering
};

// =============================================================================
// Runtime - Core ECS runtime with phases and arenas
// =============================================================================
//
// The Runtime owns the flecs world and manages phase-based execution.
// Each phase has its own arena that resets automatically after systems run.
//
// PHASES:
//   - Input:   Runs on _input(), resets after input handling
//   - Physics: Runs on _physics_process(), resets after game logic
//   - Process: Runs on _process(), resets after visuals/animation
//
// ARENAS:
//   - Phase arenas: Reset after each phase (temporary data)
//   - Permanent arena: Never resets (long-lived data)
//
// USAGE (C++):
//   Runtime* rt = Runtime::get();
//   Arena* arena = &rt->get_phase_arena(Phase_Physics);
//   void* temp = arena_alloc(arena, 256, 8);
//
// USAGE (GDScript):
//   var runtime = Engine.get_singleton("Runtime")
//   print(runtime.get_entity_count())
//
// =============================================================================

class Runtime : public Object {
    GDCLASS(Runtime, Object)
private:
    static inline Runtime* singleton_instance = nullptr;

    flecs::world m_world;
    Phase m_phases[Phase_Count];
    Arena m_permanent;

    uint64_t m_frame;          // Total _process() frames
    uint64_t m_physics_frame;  // Total _physics_process() frames

    Runtime();

protected:
    static void _bind_methods();

public:
    ~Runtime();

    // =========================================================================
    // World Access
    // =========================================================================

    /// Get the flecs world (C++ only)
    flecs::world& world() { return m_world; }
    const flecs::world& world() const { return m_world; }

    // =========================================================================
    // Phase Access
    // =========================================================================

    /// Get a phase by type
    Phase& get_phase(PhaseType type) { return m_phases[type]; }
    const Phase& get_phase(PhaseType type) const { return m_phases[type]; }

    /// Get phase arena
    Arena& get_phase_arena(PhaseType type) { return m_phases[type].arena; }
    const Arena& get_phase_arena(PhaseType type) const { return m_phases[type].arena; }

    /// Get permanent arena
    Arena& get_permanent_arena() { return m_permanent; }
    const Arena& get_permanent_arena() const { return m_permanent; }

    // =========================================================================
    // Frame Counters
    // =========================================================================

    uint64_t get_frame() const { return m_frame; }
    uint64_t get_physics_frame() const { return m_physics_frame; }

    // =========================================================================
    // GDScript API
    // =========================================================================

    /// Get current entity count (exposed to GDScript)
    int get_entity_count() const;

    // =========================================================================
    // Phase Execution
    // =========================================================================

    /// Run all systems for a phase and reset its arena
    void tick(PhaseType phase, float delta);

    // =========================================================================
    // Singleton
    // =========================================================================

    static Runtime* get() noexcept { return singleton_instance; }
    static Runtime* create_global_instance();
    static void destroy_global_instance();
};

} // namespace Polaris

#endif // POLARIS_RUNTIME_H
