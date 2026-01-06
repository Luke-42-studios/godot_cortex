#include "CompositionFactory.h"
#include "Engine.h"
#include "ECSWorld.h"
#include "util/Log.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/main_loop.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace Polaris {

using namespace godot;

// =============================================================================
// Class Registration
// =============================================================================

void CompositionFactory::_bind_methods() {
    // Debug property
    ClassDB::bind_method(D_METHOD("set_debug_enabled", "enabled"), &CompositionFactory::set_debug_enabled);
    ClassDB::bind_method(D_METHOD("get_debug_enabled"), &CompositionFactory::get_debug_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_enabled"), "set_debug_enabled", "get_debug_enabled");

    // Read-only entity count
    ClassDB::bind_method(D_METHOD("get_entity_count"), &CompositionFactory::get_entity_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "entity_count", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY), "", "get_entity_count");

    // Signal handlers (need to be bound for Callable)
    ClassDB::bind_method(D_METHOD("_on_node_added", "node"), &CompositionFactory::_on_node_added);
    ClassDB::bind_method(D_METHOD("_on_node_removed", "node"), &CompositionFactory::_on_node_removed);
    ClassDB::bind_method(D_METHOD("_try_auto_bind"), &CompositionFactory::_try_auto_bind);

    // Manual binding
    ClassDB::bind_method(D_METHOD("bind_to_scene_tree", "tree"), &CompositionFactory::bind_to_scene_tree);
    ClassDB::bind_method(D_METHOD("unbind_from_scene_tree"), &CompositionFactory::unbind_from_scene_tree);
}

// =============================================================================
// Construction / Destruction
// =============================================================================

CompositionFactory::CompositionFactory() {
    Log::info("[CompositionFactory] Created");
}

CompositionFactory::~CompositionFactory() {
    unbind_from_scene_tree();
}

// =============================================================================
// Godot Lifecycle
// =============================================================================

void CompositionFactory::_ready() {
    Log::info("[CompositionFactory] Ready - attempting auto-bind");
    _try_auto_bind();
}

void CompositionFactory::_exit_tree() {
    Log::info("[CompositionFactory] Exiting tree - unbinding");
    unbind_from_scene_tree();
}

// =============================================================================
// Scene Tree Binding
// =============================================================================

void CompositionFactory::bind_to_scene_tree(SceneTree* tree) {
    if (!tree || m_scene_tree == tree) return;

    // Skip if running in editor
    if (Engine::get_singleton()->is_editor_hint()) {
        Log::print(m_debug_enabled, "[CompositionFactory] Skipping bind - running in editor");
        return;
    }

    // Unbind from previous tree if any
    unbind_from_scene_tree();

    m_scene_tree = tree;

    // Connect to scene tree signals
    tree->connect("node_added", Callable(this, "_on_node_added"));
    tree->connect("node_removed", Callable(this, "_on_node_removed"));

    Log::info("[CompositionFactory] Bound to SceneTree");

    // Collect any nodes already in the tree
    Window* root = tree->get_root();
    if (root) {
        _collect_existing_nodes(static_cast<Node*>(root));
    }

    Log::info("[CompositionFactory] Collected existing nodes, entity_count=", m_entity_count);
}

void CompositionFactory::unbind_from_scene_tree() {
    if (!m_scene_tree) return;

    if (m_scene_tree->is_connected("node_added", Callable(this, "_on_node_added"))) {
        m_scene_tree->disconnect("node_added", Callable(this, "_on_node_added"));
    }
    if (m_scene_tree->is_connected("node_removed", Callable(this, "_on_node_removed"))) {
        m_scene_tree->disconnect("node_removed", Callable(this, "_on_node_removed"));
    }

    Log::info("[CompositionFactory] Unbound from SceneTree");
    m_scene_tree = nullptr;
}

// =============================================================================
// Auto-Binding
// =============================================================================

void CompositionFactory::_try_auto_bind() {
    if (m_scene_tree) return;

    // Skip editor
    if (Engine::get_singleton()->is_editor_hint()) {
        Log::print(m_debug_enabled, "[CompositionFactory] Skipping auto-bind - running in editor");
        return;
    }

    // Get SceneTree from main loop
    MainLoop* main_loop = Engine::get_singleton()->get_main_loop();
    SceneTree* tree = Object::cast_to<SceneTree>(main_loop);

    if (tree) {
        Log::print(m_debug_enabled, "[CompositionFactory] SceneTree found, binding...");
        bind_to_scene_tree(tree);
    } else {
        // SceneTree not ready yet, try again next frame
        Log::print(m_debug_enabled, "[CompositionFactory] SceneTree not ready, deferring...");
        call_deferred("_try_auto_bind");
    }
}

// =============================================================================
// Node Handlers
// =============================================================================

void CompositionFactory::_on_node_added(Node* node) {
    if (!node) return;

    // Check for composition metadata
    if (!node->has_meta("composition")) return;

    Variant comp_var = node->get_meta("composition");
    Ref<Composition> comp = comp_var;

    if (!comp.is_valid()) {
        Log::error("[CompositionFactory] Invalid composition on node: ", node->get_name());
        return;
    }

    // Get ECS world
    ECSWorld* ecs = ECSWorld::get();
    if (!ecs) {
        Log::error("[CompositionFactory] ECSWorld singleton not available");
        return;
    }

    String node_name = node->get_name();
    String node_class = node->get_class();

    Log::info("[CompositionFactory] +++ Node added with composition: ", node_name, " (", node_class, ")");

    // Create flecs entity
    flecs::entity entity = ecs->world().entity();
    uint64_t entity_id = entity.id();

    // Store entity ID on node
    node->set_meta("entity_id", entity_id);
    m_entity_count++;

    // Call composition to add components
    comp->compose(entity, node);

    Log::print(m_debug_enabled, "[CompositionFactory] Created entity #", entity_id, " for node: ", node_name);
}

void CompositionFactory::_on_node_removed(Node* node) {
    if (!node) return;

    // Check for entity ID
    if (!node->has_meta("entity_id")) return;

    uint64_t id = node->get_meta("entity_id");
    String node_name = node->get_name();

    Log::info("[CompositionFactory] --- Node removed with entity: ", node_name, " (entity #", id, ")");

    // Get ECS world
    ECSWorld* ecs = ECSWorld::get();
    if (!ecs) {
        Log::error("[CompositionFactory] ECSWorld singleton not available for cleanup");
        return;
    }

    // Get entity from world
    flecs::entity entity = ecs->world().entity(id);
    if (entity.is_valid()) {
        // Call decompose if composition is still available
        if (node->has_meta("composition")) {
            Variant comp_var = node->get_meta("composition");
            Ref<Composition> comp = comp_var;
            if (comp.is_valid()) {
                comp->decompose(entity);
            }
        }

        // Destroy the entity
        entity.destruct();
        m_entity_count--;
    }

    Log::print(m_debug_enabled, "[CompositionFactory] Destroyed entity #", id);
}

// =============================================================================
// Collecting Existing Nodes
// =============================================================================

void CompositionFactory::_collect_existing_nodes(Node* root) {
    if (!root) return;

    // Process this node
    if (root->has_meta("composition")) {
        _on_node_added(root);
    }

    // Recurse to children
    int child_count = root->get_child_count();
    for (int i = 0; i < child_count; i++) {
        _collect_existing_nodes(root->get_child(i));
    }
}

} // namespace Polaris
