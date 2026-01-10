#ifndef POLARIS_PIPELINE_NODE_H
#define POLARIS_PIPELINE_NODE_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <flecs.h>

namespace Polaris {

using namespace godot;

// Callback type for mouse motion events
using MouseDeltaCallback = void (*)(float dx, float dy);

// =============================================================================
// PipelineNode - Drives ECS pipelines from Godot frame callbacks
// =============================================================================
//
// Receives Godot's frame callbacks (_input, _physics_process, _process) and
// runs the corresponding ECS pipelines.
//
// PIPELINES:
//   - Input: Runs on _input() callback
//   - Physics: Runs on _physics_process() callback
//   - Process: Runs on _process() callback
//   - Render: Runs after Process pipeline
//
// USAGE:
//   Add to scene as part of Polaris autoload.
//   Pipelines run automatically each frame.
//
// =============================================================================

class PipelineNode : public Node {
    GDCLASS(PipelineNode, Node)

private:
    uint64_t m_physics_frame = 0;
    uint64_t m_process_frame = 0;
    bool m_debug_enabled = false;

    // Input callbacks (game code registers these)
    static MouseDeltaCallback s_mouse_delta_callback;

protected:
    static void _bind_methods();

public:
    PipelineNode();
    ~PipelineNode();

    // =========================================================================
    // Input Callbacks (game code registers these)
    // =========================================================================

    static void set_mouse_delta_callback(MouseDeltaCallback cb) { s_mouse_delta_callback = cb; }

    // =========================================================================
    // Godot Lifecycle
    // =========================================================================

    void _ready() override;
    void _input(const Ref<InputEvent>& event) override;
    void _physics_process(double delta) override;
    void _process(double delta) override;

    // =========================================================================
    // Properties (exposed to Godot)
    // =========================================================================

    void set_debug_enabled(bool enabled) { m_debug_enabled = enabled; }
    bool get_debug_enabled() const { return m_debug_enabled; }

    uint64_t get_physics_frame() const { return m_physics_frame; }
    uint64_t get_process_frame() const { return m_process_frame; }
};

} // namespace Polaris

#endif // POLARIS_PIPELINE_NODE_H
