#include "PipelineNode.h"
#include "Engine.h"
#include "util/Log.h"

#include <godot_cpp/classes/engine.hpp>

namespace Polaris {

using namespace godot;

// =============================================================================
// Class Registration
// =============================================================================

void PipelineNode::_bind_methods() {
    // Debug property
    ClassDB::bind_method(D_METHOD("set_debug_enabled", "enabled"), &PipelineNode::set_debug_enabled);
    ClassDB::bind_method(D_METHOD("get_debug_enabled"), &PipelineNode::get_debug_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_enabled"), "set_debug_enabled", "get_debug_enabled");

    // Read-only frame counters
    ClassDB::bind_method(D_METHOD("get_physics_frame"), &PipelineNode::get_physics_frame);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "physics_frame", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY), "", "get_physics_frame");

    ClassDB::bind_method(D_METHOD("get_process_frame"), &PipelineNode::get_process_frame);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "process_frame", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY), "", "get_process_frame");
}

// =============================================================================
// Construction / Destruction
// =============================================================================

PipelineNode::PipelineNode() {
    Log::info("[PipelineNode] Created");
}

PipelineNode::~PipelineNode() {
    Log::info("[PipelineNode] Destroyed");
}

// =============================================================================
// Godot Lifecycle
// =============================================================================

void PipelineNode::_ready() {
    Log::info("[PipelineNode] Ready");

    // Skip processing in editor
    if (godot::Engine::get_singleton()->is_editor_hint()) {
        Log::print(m_debug_enabled, "[PipelineNode] Skipping - running in editor");
        set_process(false);
        set_physics_process(false);
        set_process_input(false);
        return;
    }

    // Enable processing
    set_process(true);
    set_physics_process(true);
    set_process_input(true);
}

void PipelineNode::_input(const Ref<InputEvent>& event) {
    // TODO: Run input pipeline
    // auto& world = ECSWorld::get()->world();
    // world.progress(0.0f);

    Log::print(m_debug_enabled, "[PipelineNode] _input()");
}

void PipelineNode::_physics_process(double delta) {
    m_physics_frame++;

    // TODO: Run physics pipeline
    // auto& world = ECSWorld::get()->world();
    // world.progress(delta);

    Log::print(m_debug_enabled, "[PipelineNode] _physics_process(", delta, ") frame=", m_physics_frame);
}

void PipelineNode::_process(double delta) {
    m_process_frame++;

    // TODO: Run process and render pipelines
    // auto& world = ECSWorld::get()->world();
    // world.progress(delta);

    Log::print(m_debug_enabled, "[PipelineNode] _process(", delta, ") frame=", m_process_frame);
}

} // namespace Polaris
