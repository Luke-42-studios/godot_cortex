#include "Runtime.h"
#include "util/Log.h"

#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <mutex>

namespace Polaris {

using namespace godot;

// =============================================================================
// Class Registration
// =============================================================================

void Runtime::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_entity_count"), &Runtime::get_entity_count);
    ClassDB::bind_method(D_METHOD("get_frame"), &Runtime::get_frame);
    ClassDB::bind_method(D_METHOD("get_physics_frame"), &Runtime::get_physics_frame);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "entity_count", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY), "", "get_entity_count");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "frame", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY), "", "get_frame");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "physics_frame", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY), "", "get_physics_frame");
}

// =============================================================================
// Construction / Destruction
// =============================================================================

Runtime::Runtime() 
    : m_frame(0)
    , m_physics_frame(0)
{
    Log::info("[Runtime] Initializing...");

    // Create permanent arena (4 MB)
    m_permanent = arena_create(4 * 1024 * 1024);

    // Create phase arenas with default sizes
    m_phases[Phase_Input].arena = arena_create(64 * 1024);        // 64 KB
    m_phases[Phase_Physics].arena = arena_create(1024 * 1024);    // 1 MB
    m_phases[Phase_Process].arena = arena_create(512 * 1024);     // 512 KB

    // Create phase entities for system ordering
    // Systems register to phases with .kind(phase) and can create sub-phases
    // that depend on these main phases for ordering within the phase
    m_phases[Phase_Input].id = m_world.entity("Phase_Input")
        .add(flecs::Phase)
        .depends_on(flecs::OnUpdate);

    m_phases[Phase_Physics].id = m_world.entity("Phase_Physics")
        .add(flecs::Phase)
        .depends_on(m_phases[Phase_Input].id);

    m_phases[Phase_Process].id = m_world.entity("Phase_Process")
        .add(flecs::Phase)
        .depends_on(m_phases[Phase_Physics].id);

    Log::info("[Runtime] Initialized successfully");
    Log::info("[Runtime]   Input arena:   ", arena_remaining(&m_phases[Phase_Input].arena), " bytes");
    Log::info("[Runtime]   Physics arena: ", arena_remaining(&m_phases[Phase_Physics].arena), " bytes");
    Log::info("[Runtime]   Process arena: ", arena_remaining(&m_phases[Phase_Process].arena), " bytes");
    Log::info("[Runtime]   Permanent arena: ", arena_remaining(&m_permanent), " bytes");
}

Runtime::~Runtime() {
    Log::info("[Runtime] Destroying...");

    // Destroy arenas
    arena_destroy(&m_permanent);
    for (int i = 0; i < Phase_Count; i++) {
        arena_destroy(&m_phases[i].arena);
    }

    Log::info("[Runtime] Destroyed");
}

// =============================================================================
// GDScript API
// =============================================================================

int Runtime::get_entity_count() const {
    const ecs_world_info_t* info = ecs_get_world_info(m_world.c_ptr());
    return info ? static_cast<int>(info->last_component_id) : 0;
}

// =============================================================================
// Phase Execution
// =============================================================================

void Runtime::tick(PhaseType phase, float delta) {
    // Only run ECS systems during physics phase
    // The default flecs pipeline handles all phase dependencies correctly,
    // including sub-phases created by game systems
    //
    // We only call progress() once per frame to prevent systems from
    // running multiple times (input, physics, and process all call tick)
    if (phase == Phase_Physics) {
        m_world.progress(delta);
        m_physics_frame++;
    } else if (phase == Phase_Process) {
        m_frame++;
    }

    // Reset phase arena for next frame
    arena_reset(&m_phases[phase].arena);
}

// =============================================================================
// Singleton Management
// =============================================================================

Runtime* Runtime::create_global_instance() {
    static std::once_flag init_once;

    std::call_once(init_once, []() {
        singleton_instance = memnew(Runtime);
        // Register as Godot singleton for GDScript access
        godot::Engine::get_singleton()->register_singleton("Runtime", singleton_instance);
        Log::info("[Runtime] Registered as Godot singleton");
    });

    return singleton_instance;
}

void Runtime::destroy_global_instance() {
    if (singleton_instance) {
        godot::Engine::get_singleton()->unregister_singleton("Runtime");
        memdelete(singleton_instance);
        singleton_instance = nullptr;
    }
}

} // namespace Polaris
