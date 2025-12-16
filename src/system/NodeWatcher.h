#ifndef POLARIS_SYSTEM_NODE_WATCHER_H
#define POLARIS_SYSTEM_NODE_WATCHER_H

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/node.hpp>

#include <functional>
#include <mutex>

#include "../Log.h"

namespace Polaris {
namespace System {

using namespace godot;

/// NodeWatcher - Thin Scene Tree Event Emitter
///
/// Watches the Godot scene tree for node additions/removals and fires
/// callbacks. Does NOT store any node data - that's the ECS's job.
///
/// Computes metadata (depth, tree_id) on-demand when events fire.
///
class NodeWatcher : public Object {
    GDCLASS(NodeWatcher, Object)

public:
    using FNodeAddedCallback = std::function<void(Node* node, uint16_t depth, uint32_t tree_id)>;
    using FNodeRemovedCallback = std::function<void(Node* node)>;

private:
    static inline NodeWatcher* singleton_instance = nullptr;

    SceneTree* m_scene_tree = nullptr;
    bool m_debug_enabled = true;

    FNodeAddedCallback m_on_added_cb;
    FNodeRemovedCallback m_on_removed_cb;

    NodeWatcher(const NodeWatcher&) = delete;

protected:
    static void _bind_methods();

public:
    NodeWatcher();
    ~NodeWatcher();

    // =========================================================================
    // Debug Control
    // =========================================================================

    void set_debug_enabled(bool enabled) { m_debug_enabled = enabled; }
    bool get_debug_enabled() const { return m_debug_enabled; }

    // =========================================================================
    // Scene Tree Binding
    // =========================================================================

    void bind_to_scene_tree(SceneTree* tree);
    void unbind_from_scene_tree();

    [[nodiscard]] bool is_bound() const noexcept { return m_scene_tree != nullptr; }
    [[nodiscard]] SceneTree* get_scene_tree() const noexcept { return m_scene_tree; }

    // =========================================================================
    // Callback Registration
    // =========================================================================

    void set_on_node_added(FNodeAddedCallback cb) { m_on_added_cb = std::move(cb); }
    void set_on_node_removed(FNodeRemovedCallback cb) { m_on_removed_cb = std::move(cb); }

    // =========================================================================
    // Singleton
    // =========================================================================

    static NodeWatcher* get_singleton() noexcept { return singleton_instance; }
    static NodeWatcher* create_global_instance();
    static void destroy_global_instance();

    // =========================================================================
    // Internal
    // =========================================================================

    void _try_auto_bind();

private:
    void _on_node_added(Node* node);
    void _on_node_removed(Node* node);
    void _collect_existing_nodes(Node* root, uint32_t tree_id);

    [[nodiscard]] static uint16_t _compute_depth(Node* node, Node* tree_root);
    [[nodiscard]] static uint32_t _compute_tree_id(SceneTree* tree);
};

} // namespace System
} // namespace Polaris

#endif // POLARIS_SYSTEM_NODE_WATCHER_H
