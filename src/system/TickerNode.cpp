#include "TickerNode.h"
#include "FrameTicker.h"
#include "../Engine.h"
#include <godot_cpp/classes/engine.hpp>

namespace Polaris {
namespace System {

using namespace godot;

// =============================================================================
// Class Registration
// =============================================================================

void TickerNode::_bind_methods() {
    // Properties
    ClassDB::bind_method(D_METHOD("set_debug_enabled", "enabled"), &TickerNode::set_debug_enabled);
    ClassDB::bind_method(D_METHOD("get_debug_enabled"), &TickerNode::get_debug_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_enabled"), "set_debug_enabled", "get_debug_enabled");

    ClassDB::bind_method(D_METHOD("set_physics_enabled", "enabled"), &TickerNode::set_physics_enabled);
    ClassDB::bind_method(D_METHOD("get_physics_enabled"), &TickerNode::get_physics_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "physics_enabled"), "set_physics_enabled", "get_physics_enabled");

    ClassDB::bind_method(D_METHOD("set_process_enabled_flag", "enabled"), &TickerNode::set_process_enabled_flag);
    ClassDB::bind_method(D_METHOD("get_process_enabled_flag"), &TickerNode::get_process_enabled_flag);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "process_enabled_flag"), "set_process_enabled_flag", "get_process_enabled_flag");

    // Frame data accessors
    ClassDB::bind_method(D_METHOD("get_physics_frame_count"), &TickerNode::get_physics_frame_count);
    ClassDB::bind_method(D_METHOD("get_process_frame_count"), &TickerNode::get_process_frame_count);
    ClassDB::bind_method(D_METHOD("get_physics_time"), &TickerNode::get_physics_time);
    ClassDB::bind_method(D_METHOD("get_process_time"), &TickerNode::get_process_time);

    // Signal callback
    ClassDB::bind_method(D_METHOD("_on_tree_exiting"), &TickerNode::_on_tree_exiting);
}

// =============================================================================
// Construction / Destruction
// =============================================================================

TickerNode::TickerNode() {
    singleton_instance = this;
    set_name("PolarisTicker");
    Log::info("[Polaris::System::TickerNode] Created");
}

TickerNode::~TickerNode() {
    // Clear world pointer - don't access ECS during destruction
    m_world = nullptr;

    if (singleton_instance == this) {
        singleton_instance = nullptr;
    }
}

// =============================================================================
// Notification Handler
// =============================================================================

void TickerNode::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY: {
            // Skip processing in editor
            if (Engine::get_singleton()->is_editor_hint()) {
                set_physics_process(false);
                set_process(false);
                Log::info("[Polaris::System::TickerNode] Disabled in editor");
                return;
            }

            // Connect to tree_exiting to detect shutdown early
            get_tree()->connect("tree_exiting", Callable(this, "_on_tree_exiting"));

            // Enable physics processing
            set_physics_process(m_physics_enabled);
            set_process(m_process_enabled);

            Log::info("[Polaris::System::TickerNode] Ready, physics=", m_physics_enabled, " process=", m_process_enabled);
        } break;

        case NOTIFICATION_EXIT_TREE: {
            // Signal engine to skip node cleanup - we're shutting down
            auto* engine = PolarisEngine::get_singleton();
            if (engine) {
                engine->set_shutting_down(true);
            }

            // Stop all processing immediately
            set_physics_process(false);
            set_process(false);
            m_world = nullptr;  // Don't access world after this
            Log::info("[Polaris::System::TickerNode] Exiting tree, processing stopped");
        } break;
    }
}

// =============================================================================
// Configuration
// =============================================================================

void TickerNode::set_physics_enabled(bool enabled) {
    m_physics_enabled = enabled;
    if (is_inside_tree()) {
        set_physics_process(enabled);
    }
}

void TickerNode::set_process_enabled_flag(bool enabled) {
    m_process_enabled = enabled;
    if (is_inside_tree()) {
        set_process(enabled);
    }
}

// =============================================================================
// Initialization
// =============================================================================

void TickerNode::initialize(flecs::world* world) {
    if (!world) {
        Log::info("[Polaris::System::TickerNode] ERROR: Null world provided");
        return;
    }

    m_world = world;

    // Register singleton components for frame data
    m_world->set<PhysicsFrame>({});
    m_world->set<ProcessFrame>({});
    m_world->set<CurrentPhase>({ .phase = FramePhase::None });

    Log::info("[Polaris::System::TickerNode] Initialized with ECS world");
}

// =============================================================================
// Frame Callbacks
// =============================================================================

void TickerNode::_physics_process(double delta) {
    if (!m_world) return;

    // Skip if engine is shutting down
    auto* engine = PolarisEngine::get_singleton();
    if (!engine || engine->is_shutting_down()) return;

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

    // Progress the world - this runs all ECS systems
    if (m_physics_frame == 1) {
        Log::info("[Polaris::System::TickerNode] First physics progress() call");
    }
    m_world->progress(delta);

    // Clear phase
    m_world->set<CurrentPhase>({ .phase = FramePhase::None });

    if (m_debug_enabled && (m_physics_frame % 60 == 0)) {
        Log::info("[Polaris::System::TickerNode] Physics frame ", m_physics_frame,
                  " delta=", delta, " time=", m_physics_time);
    }
}

void TickerNode::_process(double delta) {
    if (!m_world || !m_process_enabled) return;

    // Skip if engine is shutting down
    auto* engine = PolarisEngine::get_singleton();
    if (!engine || engine->is_shutting_down()) return;

    // Update frame tracking
    m_process_frame++;
    m_process_time += delta;

    // Update the singleton component
    m_world->set<ProcessFrame>({
        .delta = delta,
        .frame = m_process_frame,
        .time = m_process_time
    });

    m_world->set<CurrentPhase>({ .phase = FramePhase::Process });

    m_world->progress(delta);

    m_world->set<CurrentPhase>({ .phase = FramePhase::None });

    if (m_debug_enabled && (m_process_frame % 60 == 0)) {
        Log::info("[Polaris::System::TickerNode] Process frame ", m_process_frame,
                  " delta=", delta, " time=", m_process_time);
    }
}

void TickerNode::_input(const Ref<InputEvent>& event) {
    if (!m_world) return;

    // TODO: Route input events to ECS
    // For now, just log in debug mode
    if (m_debug_enabled) {
        Log::info("[Polaris::System::TickerNode] Input event received");
    }
}

void TickerNode::_on_tree_exiting() {
    Log::info("[Polaris::System::TickerNode] Tree exiting - stopping processing");

    // Signal engine to skip node cleanup
    auto* engine = PolarisEngine::get_singleton();
    if (engine) {
        engine->set_shutting_down(true);
    }

    // Stop all processing immediately
    set_physics_process(false);
    set_process(false);
    m_world = nullptr;
}

} // namespace System
} // namespace Polaris
