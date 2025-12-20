#ifndef POLARIS_SYSTEM_TICKER_NODE_H
#define POLARIS_SYSTEM_TICKER_NODE_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <flecs.h>

#include "../Log.h"

namespace Polaris {
namespace System {

using namespace godot;

// =============================================================================
// TickerNode - Lightweight Node for Frame Callbacks
//
// Auto-added to the scene tree root by PolarisEngine.
// Receives _physics_process, _process, and _input callbacks.
// Routes these to the ECS world via FrameTicker.
//
// This is the "Godot way" to receive frame callbacks - more efficient
// than SceneTree signals and properly integrated with the frame timing.
// =============================================================================
class TickerNode : public Node {
    GDCLASS(TickerNode, Node)

private:
    static inline TickerNode* singleton_instance = nullptr;

    flecs::world* m_world = nullptr;
    bool m_debug_enabled = false;
    bool m_physics_enabled = true;
    bool m_process_enabled = true;  // Both physics and process run ECS systems

    // Frame counters
    uint64_t m_physics_frame = 0;
    uint64_t m_process_frame = 0;
    double m_physics_time = 0.0;
    double m_process_time = 0.0;

    TickerNode(const TickerNode&) = delete;

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    TickerNode();
    ~TickerNode();

    // =========================================================================
    // Configuration
    // =========================================================================

    void set_debug_enabled(bool enabled) { m_debug_enabled = enabled; }
    bool get_debug_enabled() const { return m_debug_enabled; }

    void set_physics_enabled(bool enabled);
    bool get_physics_enabled() const { return m_physics_enabled; }

    void set_process_enabled_flag(bool enabled);
    bool get_process_enabled_flag() const { return m_process_enabled; }

    // =========================================================================
    // Initialization
    // =========================================================================

    void initialize(flecs::world* world);

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

    static TickerNode* get_singleton() noexcept { return singleton_instance; }

    // =========================================================================
    // Node Callbacks (called by Godot)
    // =========================================================================

    void _physics_process(double delta) override;
    void _process(double delta) override;
    void _input(const Ref<InputEvent>& event) override;
    void _on_tree_exiting();
};

} // namespace System
} // namespace Polaris

#endif // POLARIS_SYSTEM_TICKER_NODE_H
