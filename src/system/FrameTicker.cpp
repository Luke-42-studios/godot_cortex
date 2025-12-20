#include "FrameTicker.h"
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/classes/time.hpp>

namespace Polaris {
namespace System {

using namespace godot;

// =============================================================================
// Class Registration
// =============================================================================

void FrameTicker::_bind_methods() {
    // Internal signal handlers
    ClassDB::bind_method(D_METHOD("_on_physics_frame"), &FrameTicker::_on_physics_frame);
    ClassDB::bind_method(D_METHOD("_on_process_frame"), &FrameTicker::_on_process_frame);

    // Properties
    ClassDB::bind_method(D_METHOD("set_debug_enabled", "enabled"), &FrameTicker::set_debug_enabled);
    ClassDB::bind_method(D_METHOD("get_debug_enabled"), &FrameTicker::get_debug_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_enabled"), "set_debug_enabled", "get_debug_enabled");

    ClassDB::bind_method(D_METHOD("set_physics_enabled", "enabled"), &FrameTicker::set_physics_enabled);
    ClassDB::bind_method(D_METHOD("get_physics_enabled"), &FrameTicker::get_physics_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "physics_enabled"), "set_physics_enabled", "get_physics_enabled");

    ClassDB::bind_method(D_METHOD("set_process_enabled", "enabled"), &FrameTicker::set_process_enabled);
    ClassDB::bind_method(D_METHOD("get_process_enabled"), &FrameTicker::get_process_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "process_enabled"), "set_process_enabled", "get_process_enabled");

    // Frame data accessors
    ClassDB::bind_method(D_METHOD("get_physics_frame_count"), &FrameTicker::get_physics_frame_count);
    ClassDB::bind_method(D_METHOD("get_process_frame_count"), &FrameTicker::get_process_frame_count);
    ClassDB::bind_method(D_METHOD("get_physics_time"), &FrameTicker::get_physics_time);
    ClassDB::bind_method(D_METHOD("get_process_time"), &FrameTicker::get_process_time);
}

// =============================================================================
// Construction / Destruction
// =============================================================================

FrameTicker::FrameTicker() {
    singleton_instance = this;
    Log::info("[Polaris::System::FrameTicker] Created");
}

FrameTicker::~FrameTicker() {
    unbind();

    if (singleton_instance == this) {
        singleton_instance = nullptr;
    }
}

// =============================================================================
// Initialization
// =============================================================================

void FrameTicker::initialize(flecs::world* world) {
    if (!world) {
        Log::info("[Polaris::System::FrameTicker] ERROR: Null world provided");
        return;
    }

    m_world = world;
    _setup_phases();

    Log::info("[Polaris::System::FrameTicker] Initialized with ECS world");
}

void FrameTicker::_setup_phases() {
    if (!m_world) return;

    // Create custom phases for physics and process
    // These are placed after OnUpdate in the default pipeline
    m_physics_phase = m_world->entity<OnPhysicsPhase>()
        .add(flecs::Phase)
        .depends_on(flecs::OnUpdate);

    m_process_phase = m_world->entity<OnProcessPhase>()
        .add(flecs::Phase)
        .depends_on(flecs::OnUpdate);

    // Register singleton components
    m_world->set<PhysicsFrame>({});
    m_world->set<ProcessFrame>({});
    m_world->set<CurrentPhase>({ .phase = FramePhase::None });

    Log::print(m_debug_enabled, "[Polaris::System::FrameTicker] Phases and singletons registered");
}

void FrameTicker::bind_to_scene_tree(SceneTree* tree) {
    if (!tree || m_scene_tree == tree) return;

    if (Engine::get_singleton()->is_editor_hint()) {
        Log::print(m_debug_enabled, "[Polaris::System::FrameTicker] Skipping bind - running in editor");
        return;
    }

    unbind();

    m_scene_tree = tree;

    // Connect to frame signals
    tree->connect("physics_frame", Callable(this, "_on_physics_frame"));
    tree->connect("process_frame", Callable(this, "_on_process_frame"));

    Log::info("[Polaris::System::FrameTicker] Bound to scene tree frame signals");
}

void FrameTicker::unbind() {
    if (!m_scene_tree) return;

    Callable physics_cb = Callable(this, "_on_physics_frame");
    Callable process_cb = Callable(this, "_on_process_frame");

    if (m_scene_tree->is_connected("physics_frame", physics_cb)) {
        m_scene_tree->disconnect("physics_frame", physics_cb);
    }
    if (m_scene_tree->is_connected("process_frame", process_cb)) {
        m_scene_tree->disconnect("process_frame", process_cb);
    }

    m_scene_tree = nullptr;
    Log::print(m_debug_enabled, "[Polaris::System::FrameTicker] Unbound from scene tree");
}

// =============================================================================
// Frame Handlers
// =============================================================================

void FrameTicker::_on_physics_frame() {
    if (!m_physics_enabled || !m_world) return;

    // Calculate physics delta from ticks per second
    int32_t ticks = Engine::get_singleton()->get_physics_ticks_per_second();
    double delta = (ticks > 0) ? (1.0 / static_cast<double>(ticks)) : (1.0 / 60.0);

    // Update frame tracking
    m_physics_frame++;
    m_physics_time += delta;

    // Update the singleton component so systems can access frame data
    m_world->set<PhysicsFrame>({
        .delta = delta,
        .frame = m_physics_frame,
        .time = m_physics_time
    });

    // Set current phase so systems know we're in physics
    m_world->set<CurrentPhase>({ .phase = FramePhase::Physics });

    // Progress the world - this runs all systems
    m_world->progress(delta);

    // Clear phase
    m_world->set<CurrentPhase>({ .phase = FramePhase::None });

    if (m_debug_enabled && (m_physics_frame % 60 == 0)) {
        Log::print(true, "[Polaris::System::FrameTicker] Physics frame ", m_physics_frame,
                   " delta=", delta, " time=", m_physics_time);
    }
}

void FrameTicker::_on_process_frame() {
    if (!m_process_enabled || !m_world) return;

    // Calculate process delta using Time singleton
    double current_time = Time::get_singleton()->get_ticks_usec() / 1000000.0;
    static double last_process_time = current_time;
    double delta = current_time - last_process_time;
    last_process_time = current_time;

    // Clamp delta to reasonable range (avoid huge deltas on first frame or hitches)
    if (delta <= 0.0 || delta > 0.5) {
        delta = 1.0 / 60.0;
    }

    // Update frame tracking
    m_process_frame++;
    m_process_time += delta;

    // Update the singleton component
    m_world->set<ProcessFrame>({
        .delta = delta,
        .frame = m_process_frame,
        .time = m_process_time
    });

    // Set current phase so systems know we're in process
    m_world->set<CurrentPhase>({ .phase = FramePhase::Process });

    // Progress the world - this runs all systems registered to OnProcessPhase
    m_world->progress(delta);

    // Clear phase
    m_world->set<CurrentPhase>({ .phase = FramePhase::None });

    if (m_debug_enabled && (m_process_frame % 60 == 0)) {
        Log::print(true, "[Polaris::System::FrameTicker] Process frame ", m_process_frame,
                   " delta=", delta, " time=", m_process_time);
    }
}

// =============================================================================
// Singleton Management
// =============================================================================

FrameTicker* FrameTicker::create_global_instance() {
    static std::once_flag init_once;

    std::call_once(init_once, []() {
        singleton_instance = memnew(FrameTicker);
        Engine::get_singleton()->register_singleton("FrameTicker", singleton_instance);
    });

    return singleton_instance;
}

void FrameTicker::destroy_global_instance() {
    if (singleton_instance) {
        Engine::get_singleton()->unregister_singleton("FrameTicker");
        memdelete(singleton_instance);
        singleton_instance = nullptr;
    }
}

} // namespace System
} // namespace Polaris
