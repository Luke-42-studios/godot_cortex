#ifndef NODE_WATCHER_H
#define NODE_WATCHER_H

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

/// --------------------------------------------------------------------
/// NodeWatcher – watches the whole scene‑tree, keeps a flat list of
/// live nodes and can **optionally** dump that list to the Godot debugger each frame.
/// --------------------------------------------------------------------
class NodeWatcher : public Object {
    GDCLASS(NodeWatcher, Object)

private:
    static inline NodeWatcher *singleton_instance = nullptr;

    // ----- core data ---------------------------------------------------
    Vector<Node *> nodes;          // packed array of raw Node* (no ref‑count)
    SceneTree *scene_tree = nullptr;

    static void _bind_methods();

    // Prevent copying
    NodeWatcher(const NodeWatcher&) = delete;

    // Signal callbacks
    void _on_node_added(Node *node);
    void _on_node_removed(Node *node);

    // Recursively collect all nodes from a root
    void _collect_nodes(Node *root);

    void _on_stack_changed();

    // Attempts to auto-bind to the scene tree
    void _try_auto_bind();

public:
    // Bind to scene tree and start watching
    void bind_to_scene_tree(SceneTree *tree);

    // Check if currently bound to a scene tree
    bool is_bound() const { return scene_tree != nullptr; }
    // Fast C++ access (avoids Engine string lookup)
    static NodeWatcher* get_singleton() noexcept {
        return singleton_instance;
    }

    // ----------------------------------------------------------------
    // Construction / destruction
    // ----------------------------------------------------------------
    NodeWatcher();
    ~NodeWatcher();

    void _process(double delta);

    // Static methods for module integration
    static NodeWatcher* create_global_instance();
    static void destroy_global_instance();

    /// Return a copy of the current node list (useful for your own debugging).
    Vector<Node *> get_current_view() const { return nodes; }

    /// Manual dump – also called automatically each frame when `debug_enabled` is true.
    void debug_print_tree() const;
};

#endif // NODE_WATCHER_H
