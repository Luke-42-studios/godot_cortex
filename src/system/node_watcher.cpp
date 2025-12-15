// NodeWatcher.cpp
#include "node_watcher.h"
#include <functional>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/main_loop.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

/* --------------------------------------------------------------
   Debug print – called automatically each frame when `debug_enabled`
   or manually via `debug_print_tree()`.
   -------------------------------------------------------------- */
void NodeWatcher::debug_print_tree() const {
    if (nodes.is_empty()) {
        UtilityFunctions::print("[NodeWatcher] <empty>");
        return;
    }

    UtilityFunctions::print("[NodeWatcher] ----------------------------------");

    // Depth-first tree printer showing name and type
    std::function<void(Node *, int, bool)> recurse = [&](Node *n, int depth, bool is_last) {
        String indent;
        for (int i = 0; i < depth - 1; ++i) {
            indent += "|   ";
        }
        if (depth > 0) {
            indent += is_last ? "\\-- " : "+-- ";
        }

        String node_type = n->get_class();
        String node_name = n->get_name();
        UtilityFunctions::print(indent, node_name, " [", node_type, "]");

        int child_count = n->get_child_count();
        for (int i = 0; i < child_count; ++i) {
            recurse(n->get_child(i), depth + 1, i == child_count - 1);
        }
    };

    // Start at the real root of the current tree.
    if (nodes[0] && nodes[0]->get_tree()) {
        Node *real_root = Object::cast_to<Node>(nodes[0]->get_tree()->get_root());
        if (real_root) recurse(real_root, 0, true);
    }

    UtilityFunctions::print("[NodeWatcher] Total nodes: ", nodes.size());
    UtilityFunctions::print("[NodeWatcher] ----------------------------------");
}

/* --------------------------------------------------------------
   GDCLASS registration – makes the class visible to GDScript.
   -------------------------------------------------------------- */
void NodeWatcher::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_node_added", "node"), &NodeWatcher::_on_node_added);
    ClassDB::bind_method(D_METHOD("_on_node_removed", "node"), &NodeWatcher::_on_node_removed);
    ClassDB::bind_method(D_METHOD("_try_auto_bind"), &NodeWatcher::_try_auto_bind);
}

/* --------------------------------------------------------------
   Scene tree binding and node tracking
   -------------------------------------------------------------- */
void NodeWatcher::bind_to_scene_tree(SceneTree *tree) {
    if (!tree) return;

    scene_tree = tree;
    nodes.clear();

    // Connect to scene tree signals
    tree->connect("node_added", Callable(this, "_on_node_added"));
    tree->connect("node_removed", Callable(this, "_on_node_removed"));

    // Collect all existing nodes
    if (tree->get_root()) {
        _collect_nodes(Object::cast_to<Node>(tree->get_root()));
    }

    UtilityFunctions::print("[NodeWatcher] Bound to scene tree, tracking ", nodes.size(), " nodes");

    // Print initial tree state
    _on_stack_changed();
}

void NodeWatcher::_collect_nodes(Node *root) {
    if (!root) return;

    nodes.push_back(root);

    for (int i = 0; i < root->get_child_count(); ++i) {
        _collect_nodes(root->get_child(i));
    }
}

void NodeWatcher::_on_node_added(Node *node) {
    if (!node) return;
    nodes.push_back(node);
    _on_stack_changed();
}

void NodeWatcher::_on_node_removed(Node *node) {
    if (!node) return;
    int idx = nodes.find(node);
    if (idx >= 0) {
        nodes.remove_at(idx);
    }

    _on_stack_changed();
}

void NodeWatcher::_on_stack_changed() {
    debug_print_tree();
}

void NodeWatcher::_try_auto_bind() {
    if (scene_tree) return; // Already bound

    // Try to get SceneTree from Engine's main loop
    MainLoop *main_loop = Engine::get_singleton()->get_main_loop();
    SceneTree *tree = Object::cast_to<SceneTree>(main_loop);

    if (tree) {
        UtilityFunctions::print("[NodeWatcher] SceneTree found, binding...");
        bind_to_scene_tree(tree);
    } else {
        UtilityFunctions::print("[NodeWatcher] SceneTree not ready, deferring...");
        call_deferred("_try_auto_bind");
    }
}

/* --------------------------------------------------------------
   Construction / destruction
   -------------------------------------------------------------- */
NodeWatcher::NodeWatcher() {
    singleton_instance = this;
    UtilityFunctions::print("[NodeWatcher] Init");
}

NodeWatcher::~NodeWatcher() {
    // Disconnect from scene tree signals
    if (scene_tree) {
        scene_tree->disconnect("node_added", Callable(this, "_on_node_added"));
        scene_tree->disconnect("node_removed", Callable(this, "_on_node_removed"));
    }
    nodes.clear();

    if (singleton_instance == this)
        singleton_instance = nullptr;
}

void NodeWatcher::_process(double delta) {
    // Process logic here
}

// Global instance management
NodeWatcher* NodeWatcher::create_global_instance() {
    if (!singleton_instance) {
        singleton_instance = memnew(NodeWatcher);
        // Register as a singleton with the engine so it's accessible globally
        Engine::get_singleton()->register_singleton("NodeWatcher", singleton_instance);
        // Auto-bind to scene tree (deferred if not ready yet)
        singleton_instance->_try_auto_bind();
    }
    return singleton_instance;
}

void NodeWatcher::destroy_global_instance() {
    if (singleton_instance) {
        Engine::get_singleton()->unregister_singleton("NodeWatcher");
        memdelete(singleton_instance);
        singleton_instance = nullptr;
    }
}
