#include "PipelineNode.h"
#include "Runtime.h"
#include "Engine.h"
#include "util/Log.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>

namespace Polaris {

using namespace godot;

// Static callback storage
MouseDeltaCallback PipelineNode::s_mouse_delta_callback = nullptr;

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
    // Capture mouse motion for FPS camera
    if (s_mouse_delta_callback) {
        const InputEventMouseMotion* motion = Object::cast_to<InputEventMouseMotion>(*event);
        if (motion) {
            Vector2 rel = motion->get_relative();
            s_mouse_delta_callback(rel.x, rel.y);
        }
    }

    Runtime* rt = Runtime::get();
    if (rt) {
        rt->tick(Phase_Input, 0.0f);
    }
}

void PipelineNode::_physics_process(double delta) {
    m_physics_frame++;

    Runtime* rt = Runtime::get();
    if (rt) {
        rt->tick(Phase_Physics, (float)delta);
    }
}

void PipelineNode::_process(double delta) {
    m_process_frame++;

    Runtime* rt = Runtime::get();
    if (rt) {
        rt->tick(Phase_Process, (float)delta);
    }
}

} // namespace Polaris
