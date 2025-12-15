// NodeWatcher.cpp - Optimized for Polaris Engine conventions
#include "SNodeWatcher.h"
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/main_loop.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

// ============================================================================
// Debug Interface - Conditional compilation based on debug settings
// ============================================================================

void SNodeWatcher::debug_print_tree() const {
    if (!m_debug_enabled) return;  // COLD PATH: Skip expensive operations

    if (m_nodes.empty()) {
        UtilityFunctions::print("[SNodeWatcher] <empty>");
        return;
    }

    UtilityFunctions::print("[SNodeWatcher] ----------------------------------");

    // Count nodes during traversal for accurate reporting
    size_t node_count = 0;

    // Build tree structure efficiently
    std::function<void(Node*, int, bool)> recurse = [&](Node* n, int depth, bool is_last) {
        node_count++;

        String indent;
        for (int i = 0; i < depth - 1; ++i) {
            indent += "|   ";
        }
        if (depth > 0) {
            indent += is_last ? "\\-- " : "+-- ";
        }

        String node_name = n->get_name();

        // Look up components from tracked data
        String components_str;
        auto it = m_node_index_lookup.find(n);
        if (it != m_node_index_lookup.end()) {
            const SNodeData& data = m_nodes[it->second];
            for (size_t i = 0; i < data.components.size(); ++i) {
                if (i > 0) components_str += ", ";
                components_str += data.components[i];
            }
        } else {
            // Fallback: just show the class name
            components_str = n->get_class();
        }

        // Print node with components
        UtilityFunctions::print(indent, node_name, " [", components_str, "]");

        int child_count = n->get_child_count();
        for (int i = 0; i < child_count; ++i) {
            recurse(n->get_child(i), depth + 1, i == child_count - 1);
        }
    };

    // Start at the real root using tree_id for multi-scene support
    if (!m_nodes.empty() && m_nodes[0].node && m_nodes[0].node->get_tree()) {
        Node* real_root = Object::cast_to<Node>(m_nodes[0].node->get_tree()->get_root());
        if (real_root) recurse(real_root, 0, true);
    }

    UtilityFunctions::print("[SNodeWatcher] Total nodes: ", node_count);
    UtilityFunctions::print("[SNodeWatcher] Tracked nodes: ", m_nodes.size());
    UtilityFunctions::print("[SNodeWatcher] ----------------------------------");
}

// ============================================================================
// Class Registration
// ============================================================================

void SNodeWatcher::_bind_methods() {
    // BIND: Register methods for GDScript access if needed
    ClassDB::bind_method(D_METHOD("_on_node_added", "node"), &SNodeWatcher::_on_node_added);
    ClassDB::bind_method(D_METHOD("_on_node_removed", "node"), &SNodeWatcher::_on_node_removed);
    ClassDB::bind_method(D_METHOD("_try_auto_bind"), &SNodeWatcher::_try_auto_bind);
    ClassDB::bind_method(D_METHOD("_on_stack_changed"), &SNodeWatcher::_on_stack_changed);

    // ADD: Properties for external configuration
    ClassDB::bind_method(D_METHOD("set_debug_mode", "enabled"), &SNodeWatcher::set_debug_mode);
    ClassDB::bind_method(D_METHOD("get_debug_mode"), &SNodeWatcher::get_debug_mode);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_enabled"), "set_debug_mode", "get_debug_mode");
}

// ============================================================================
// Scene Tree Integration  
// ============================================================================

void SNodeWatcher::bind_to_scene_tree(SceneTree* tree) {
    if (!tree || m_scene_tree == tree) return;  // Already bound to this tree

    // SKIP: Don't bind in the editor
    if (Engine::get_singleton()->is_editor_hint()) {
        UtilityFunctions::print("[SNodeWatcher] Skipping bind - running in editor");
        return;
    }

    // CLEANUP: Disconnect from previous tree
    if (m_scene_tree) {
        m_scene_tree->disconnect("node_added", Callable(this, "_on_node_added"));
        m_scene_tree->disconnect("node_removed", Callable(this, "_on_node_removed"));
    }

    m_scene_tree = tree;
    m_nodes.clear();
    m_node_index_lookup.clear();

    // CONNECT: Scene tree signals for automatic tracking
    tree->connect("node_added", Callable(this, "_on_node_added"));
    tree->connect("node_removed", Callable(this, "_on_node_removed"));

    // COLLECT: Gather existing nodes (COLD PATH - done once per binding)
    if (tree->get_root()) {
        _collect_nodes(Object::cast_to<Node>(tree->get_root()), 0);
    }

    UtilityFunctions::print("[SNodeWatcher] Bound to scene tree, tracking ", m_nodes.size(), " nodes");

    // DEBUG: Print initial state only if debug is enabled
    if (m_debug_enabled) {
        _on_stack_changed();
    }
}

void SNodeWatcher::_collect_nodes(Node* root, uint32_t tree_id) {
    if (!root) return;

    // BUILD: Efficient node tracking with pre-computed metadata
    size_t depth = 0;
    Node* parent = root->get_parent();

    while (parent && parent != m_scene_tree->get_root()) {
        depth++;
        parent = parent->get_parent();
    }

    SNodeData data{
        .node = root,
        .tree_id = tree_id,
        .depth = depth,
        .is_active = true,
        .components = _get_node_components(root)
    };

    size_t index = m_nodes.size();
    m_nodes.push_back(data);
    m_node_index_lookup[root] = index;

    // Notify external listeners for initial collection
    if (m_on_added_cb) {
        m_on_added_cb(root, m_nodes.back());
    }

    // RECURSE: Collect children with same optimization approach
    for (int i = 0; i < root->get_child_count(); ++i) {
        _collect_nodes(root->get_child(i), tree_id);
    }
}

// ============================================================================
// Signal Handling - Optimized for high-frequency updates
// ============================================================================

void SNodeWatcher::_on_node_added(Node* node) {
    if (!node) return;

    // SKIP: Don't process in the editor
    if (Engine::get_singleton()->is_editor_hint()) return;

    // FAST: O(1) check to prevent duplicate tracking
    if (m_node_index_lookup.find(node) != m_node_index_lookup.end()) {
        return;  // Already tracked
    }

    // PERF: Add with pre-computed metadata
    size_t depth = _calculate_node_depth(node);

    SNodeData data{
        .node = node,
        .tree_id = _get_tree_id_for_node(node),
        .depth = depth,
        .is_active = true,
        .components = _get_node_components(node)
    };

    m_nodes.push_back(data);
    m_node_index_lookup[node] = m_nodes.size() - 1;

    // Notify external listeners (e.g., CPolaris for ECS entity creation)
    if (m_on_added_cb) {
        m_on_added_cb(node, m_nodes.back());
    }

    // DEBUG: Optional tree state update
    if (m_debug_enabled) {
        _on_stack_changed();
    }
}

void SNodeWatcher::_on_node_removed(Node* node) {
    if (!node) return;

    // SKIP: Don't process in the editor
    if (Engine::get_singleton()->is_editor_hint()) return;

    auto it = m_node_index_lookup.find(node);
    if (it == m_node_index_lookup.end()) {
        return;  // Not tracked
    }

    // Notify external listeners BEFORE removal (e.g., CPolaris for ECS entity destruction)
    if (m_on_removed_cb) {
        m_on_removed_cb(node);
    }

    size_t index = it->second;

    // PERF: Swap-remove instead of erase for O(1) removal
    size_t last_index = m_nodes.size() - 1;

    if (index != last_index) {
        std::swap(m_nodes[index], m_nodes[last_index]);
        m_node_index_lookup[m_nodes[index].node] = index;  // Update lookup table
    }

    m_nodes.pop_back();
    m_node_index_lookup.erase(it);

    // DEBUG: Defer the debug print until node is actually removed from tree
    if (m_debug_enabled) {
        call_deferred("_on_stack_changed");
    }
}

// ============================================================================
// Utility Functions  
// ============================================================================

void SNodeWatcher::_on_stack_changed() {
    // SKIP: Don't process in the editor (handles deferred calls)
    if (Engine::get_singleton()->is_editor_hint()) return;

    debug_print_tree();
}

void SNodeWatcher::unbind_from_scene_tree() {
    if (!m_scene_tree) return;

    UtilityFunctions::print("[SNodeWatcher] Unbinding from scene tree");

    Callable on_added = Callable(this, "_on_node_added");
    Callable on_removed = Callable(this, "_on_node_removed");

    if (m_scene_tree->is_connected("node_added", on_added)) {
        m_scene_tree->disconnect("node_added", on_added);
    }
    if (m_scene_tree->is_connected("node_removed", on_removed)) {
        m_scene_tree->disconnect("node_removed", on_removed);
    }

    m_scene_tree = nullptr;
    m_nodes.clear();
    m_node_index_lookup.clear();
}

bool SNodeWatcher::contains(Node* node) const {
    return m_node_index_lookup.find(node) != m_node_index_lookup.end();
}

size_t SNodeWatcher::_calculate_node_depth(Node* node) const {
    size_t depth = 0;
    Node* parent = node->get_parent();
    
    while (parent && parent != m_scene_tree->get_root()) {
        depth++;
        parent = parent->get_parent();
    }
    
    return depth;
}

uint32_t SNodeWatcher::_get_tree_id_for_node(Node* node) const {
    if (!node || !node->get_tree()) return 0;
    return reinterpret_cast<uint32_t>(node->get_tree());
}

std::vector<String> SNodeWatcher::_get_node_components(Node* node) const {
    std::vector<String> components;
    if (!node) return components;

    // Get the class inheritance chain
    // In Godot, we walk up the inheritance hierarchy
    String current_class = node->get_class();

    while (!current_class.is_empty() && current_class != "Object") {
        components.push_back(current_class);

        // Get parent class using ClassDB
        StringName parent_class = ClassDB::get_parent_class(current_class);
        if (parent_class == StringName()) break;
        current_class = String(parent_class);
    }

    return components;
}

// ============================================================================
// Auto-binding and Lifecycle Management
// ============================================================================

void SNodeWatcher::_try_auto_bind() {
    if (m_scene_tree) return; // Already bound

    // SKIP: Don't watch nodes when running in the editor
    if (Engine::get_singleton()->is_editor_hint()) {
        UtilityFunctions::print("[SNodeWatcher] Skipping - running in editor");
        return;
    }

    MainLoop* main_loop = Engine::get_singleton()->get_main_loop();
    SceneTree* tree = Object::cast_to<SceneTree>(main_loop);

    if (tree) {
        UtilityFunctions::print("[SNodeWatcher] SceneTree found, binding...");
        bind_to_scene_tree(tree);
    } else {
        // DEFER: Try again later when scene tree becomes available
        call_deferred("_try_auto_bind");
    }
}

// ============================================================================
// Godot Integration  
// ============================================================================

void SNodeWatcher::_process(double delta) {
    // OPTIMIZE: Update node active states if needed for performance monitoring
    // This is a HOT path called every frame, keep it minimal
}

// ============================================================================
// Singleton Management  
// ============================================================================

SNodeWatcher::SNodeWatcher() {
    singleton_instance = this;
    UtilityFunctions::print("[SNodeWatcher] Initialized");
}

SNodeWatcher::~SNodeWatcher() {
    // CLEANUP: Proper signal disconnection - only if actually connected
    if (m_scene_tree) {
        Callable on_added = Callable(this, "_on_node_added");
        Callable on_removed = Callable(this, "_on_node_removed");

        if (m_scene_tree->is_connected("node_added", on_added)) {
            m_scene_tree->disconnect("node_added", on_added);
        }
        if (m_scene_tree->is_connected("node_removed", on_removed)) {
            m_scene_tree->disconnect("node_removed", on_removed);
        }
    }

    if (singleton_instance == this) {
        singleton_instance = nullptr;
    }
}

SNodeWatcher* SNodeWatcher::create_global_instance() {
    static std::once_flag init_once;
    
    std::call_once(init_once, []() {
        singleton_instance = memnew(SNodeWatcher);
        Engine::get_singleton()->register_singleton("SNodeWatcher", singleton_instance);
        
        // DEFER: Auto-bind to scene tree if not immediately available  
        singleton_instance->_try_auto_bind();
    });
    
    return singleton_instance;
}

void SNodeWatcher::destroy_global_instance() {
    if (singleton_instance) {
        Engine::get_singleton()->unregister_singleton("SNodeWatcher"); 
        
        // PERF: Ensure proper cleanup order
        memdelete(singleton_instance);
        singleton_instance = nullptr;
    }
}
