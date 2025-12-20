#include "NodeWatcher.h"
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/main_loop.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace Polaris {
namespace System {

using namespace godot;

// =============================================================================
// Class Registration
// =============================================================================

void NodeWatcher::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_node_added", "node"), &NodeWatcher::_on_node_added);
    ClassDB::bind_method(D_METHOD("_on_node_removed", "node"), &NodeWatcher::_on_node_removed);
    ClassDB::bind_method(D_METHOD("_try_auto_bind"), &NodeWatcher::_try_auto_bind);

    ClassDB::bind_method(D_METHOD("set_debug_enabled", "enabled"), &NodeWatcher::set_debug_enabled);
    ClassDB::bind_method(D_METHOD("get_debug_enabled"), &NodeWatcher::get_debug_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_enabled"), "set_debug_enabled", "get_debug_enabled");
}

// =============================================================================
// Construction / Destruction
// =============================================================================

NodeWatcher::NodeWatcher() {
    singleton_instance = this;
    Log::info("[Polaris::System::NodeWatcher] Created");
}

NodeWatcher::~NodeWatcher() {
    // Don't unbind - it can cause slowdowns during shutdown
    // Just clear our pointer
    m_scene_tree = nullptr;

    if (singleton_instance == this) {
        singleton_instance = nullptr;
    }
}

// =============================================================================
// Scene Tree Binding
// =============================================================================

void NodeWatcher::bind_to_scene_tree(SceneTree* tree) {
    if (!tree || m_scene_tree == tree) return;

    if (Engine::get_singleton()->is_editor_hint()) {
        Log::print(m_debug_enabled, "[Polaris::System::NodeWatcher] Skipping bind - running in editor");
        return;
    }

    unbind_from_scene_tree();

    m_scene_tree = tree;

    tree->connect("node_added", Callable(this, "_on_node_added"));
    tree->connect("node_removed", Callable(this, "_on_node_removed"));

    Log::print(m_debug_enabled, "[Polaris::System::NodeWatcher] Connected to scene tree signals");

    Node* root = Object::cast_to<Node>(tree->get_root());
    if (root) {
        uint32_t tree_id = _compute_tree_id(tree);
        _collect_existing_nodes(root, tree_id);
    }

    Log::info("[Polaris::System::NodeWatcher] Bound to scene tree");
}

void NodeWatcher::unbind_from_scene_tree() {
    if (!m_scene_tree) return;

    Callable on_added = Callable(this, "_on_node_added");
    Callable on_removed = Callable(this, "_on_node_removed");

    if (m_scene_tree->is_connected("node_added", on_added)) {
        m_scene_tree->disconnect("node_added", on_added);
    }
    if (m_scene_tree->is_connected("node_removed", on_removed)) {
        m_scene_tree->disconnect("node_removed", on_removed);
    }

    m_scene_tree = nullptr;
    Log::print(m_debug_enabled, "[Polaris::System::NodeWatcher] Unbound from scene tree");
}

// =============================================================================
// Signal Handlers
// =============================================================================

void NodeWatcher::_on_node_added(Node* node) {
    if (!node || !m_on_added_cb) return;
    if (Engine::get_singleton()->is_editor_hint()) return;

    Node* root = m_scene_tree ? Object::cast_to<Node>(m_scene_tree->get_root()) : nullptr;
    uint16_t depth = _compute_depth(node, root);
    uint32_t tree_id = _compute_tree_id(m_scene_tree);

    m_on_added_cb(node, depth, tree_id);

    Log::print(m_debug_enabled, "[Polaris::System::NodeWatcher] Node added: ", node->get_name(),
               " (depth=", depth, ")");
}

void NodeWatcher::_on_node_removed(Node* node) {
    if (!node || !m_on_removed_cb) return;
    if (Engine::get_singleton()->is_editor_hint()) return;

    m_on_removed_cb(node);

    Log::print(m_debug_enabled, "[Polaris::System::NodeWatcher] Node removed: ", node->get_name());
}

// =============================================================================
// Initial Collection
// =============================================================================

void NodeWatcher::_collect_existing_nodes(Node* root, uint32_t tree_id) {
    if (!root) return;

    Node* tree_root = m_scene_tree ? Object::cast_to<Node>(m_scene_tree->get_root()) : nullptr;
    uint16_t depth = _compute_depth(root, tree_root);

    if (m_on_added_cb) {
        m_on_added_cb(root, depth, tree_id);
    }

    int child_count = root->get_child_count();
    for (int i = 0; i < child_count; ++i) {
        _collect_existing_nodes(root->get_child(i), tree_id);
    }
}

// =============================================================================
// Metadata Computation
// =============================================================================

uint16_t NodeWatcher::_compute_depth(Node* node, Node* tree_root) {
    if (!node) return 0;

    uint16_t depth = 0;
    Node* current = node->get_parent();

    while (current && current != tree_root) {
        depth++;
        current = current->get_parent();
    }

    return depth;
}

uint32_t NodeWatcher::_compute_tree_id(SceneTree* tree) {
    return tree ? static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tree) & 0xFFFFFFFF) : 0;
}

// =============================================================================
// Auto-Binding
// =============================================================================

void NodeWatcher::_try_auto_bind() {
    if (m_scene_tree) return;

    if (Engine::get_singleton()->is_editor_hint()) {
        Log::print(m_debug_enabled, "[Polaris::System::NodeWatcher] Skipping - running in editor");
        return;
    }

    MainLoop* main_loop = Engine::get_singleton()->get_main_loop();
    SceneTree* tree = Object::cast_to<SceneTree>(main_loop);

    if (tree) {
        Log::print(m_debug_enabled, "[Polaris::System::NodeWatcher] SceneTree found, binding...");
        bind_to_scene_tree(tree);
    } else {
        call_deferred("_try_auto_bind");
    }
}

// =============================================================================
// Singleton Management
// =============================================================================

NodeWatcher* NodeWatcher::create_global_instance() {
    // std::call_once guarantees thread-safe one-time initialization.
    // See Polaris::Context::ECSWorld for detailed explanation.
    static std::once_flag init_once;

    std::call_once(init_once, []() {
        singleton_instance = memnew(NodeWatcher);
        Engine::get_singleton()->register_singleton("NodeWatcher", singleton_instance);
    });

    return singleton_instance;
}

void NodeWatcher::destroy_global_instance() {
    if (singleton_instance) {
        Engine::get_singleton()->unregister_singleton("NodeWatcher");
        memdelete(singleton_instance);
        singleton_instance = nullptr;
    }
}

} // namespace System
} // namespace Polaris
