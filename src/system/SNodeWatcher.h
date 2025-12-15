#ifndef SNODE_WATCHER_H
#define SNODE_WATCHER_H

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <vector>
#include <unordered_map>
#include <functional>
#include <mutex>

using namespace godot;

// Forward declaration for callback signature
struct SNodeData;

/// Node tracking data structure for efficient lookups
struct SNodeData {
    Node* node = nullptr;
    uint32_t tree_id = 0;
    size_t depth = 0;
    bool is_active = true;
    std::vector<String> components;  // Class inheritance chain (e.g., MeshInstance3D -> Node3D -> Node)
};

/// --------------------------------------------------------------------
/// SNodeWatcher – watches the whole scene-tree, keeps a flat list of
/// live nodes and can optionally dump that list to the Godot debugger.
/// --------------------------------------------------------------------
class SNodeWatcher : public Object {
    GDCLASS(SNodeWatcher, Object)

public:
    // Callback types - SNodeWatcher doesn't know what they're used for
    using NodeAddedCallback = std::function<void(Node*, const SNodeData&)>;
    using NodeRemovedCallback = std::function<void(Node*)>;

private:
    static inline SNodeWatcher* singleton_instance = nullptr;

    // Core data
    std::vector<SNodeData> m_nodes;
    std::unordered_map<Node*, size_t> m_node_index_lookup;
    SceneTree* m_scene_tree = nullptr;
    bool m_debug_enabled = true;

    // External callbacks for integration (e.g., CPolaris uses these)
    NodeAddedCallback m_on_added_cb;
    NodeRemovedCallback m_on_removed_cb;

    static void _bind_methods();

    // Prevent copying
    SNodeWatcher(const SNodeWatcher&) = delete;

    // Signal callbacks
    void _on_node_added(Node* node);
    void _on_node_removed(Node* node);

    // Recursively collect all nodes from a root
    void _collect_nodes(Node* root, uint32_t tree_id);

    void _on_stack_changed();

    // Utility functions
    size_t _calculate_node_depth(Node* node) const;
    uint32_t _get_tree_id_for_node(Node* node) const;
    std::vector<String> _get_node_components(Node* node) const;

public:
    // Bind to scene tree and start watching
    void bind_to_scene_tree(SceneTree* tree);

    // Unbind from scene tree and stop watching
    void unbind_from_scene_tree();

    // Check if currently bound to a scene tree
    bool is_bound() const { return m_scene_tree != nullptr; }

    // Check if a node is being tracked
    bool contains(Node* node) const;

    // Get active node count
    size_t get_active_count() const {
        size_t count = 0;
        for (const auto& data : m_nodes) {
            if (data.is_active) count++;
        }
        return count;
    }

    // Debug mode control
    void set_debug_mode(bool enabled) { m_debug_enabled = enabled; }
    bool get_debug_mode() const { return m_debug_enabled; }

    // Callback registration - allows external systems to react to node changes
    void set_on_node_added(NodeAddedCallback cb) { m_on_added_cb = std::move(cb); }
    void set_on_node_removed(NodeRemovedCallback cb) { m_on_removed_cb = std::move(cb); }

    // Attempts to auto-bind to the scene tree (called by CPolaris after wiring callbacks)
    void _try_auto_bind();

    // Fast C++ access (avoids Engine string lookup)
    static SNodeWatcher* get_singleton() noexcept {
        return singleton_instance;
    }

    // Construction / destruction
    SNodeWatcher();
    ~SNodeWatcher();

    void _process(double delta);

    // Static methods for module integration
    static SNodeWatcher* create_global_instance();
    static void destroy_global_instance();

    /// Return a copy of the current node list
    const std::vector<SNodeData>& get_current_view() const { return m_nodes; }

    /// Manual dump – also called automatically each frame when debug_enabled is true
    void debug_print_tree() const;
};

#endif // SNODE_WATCHER_H
