#include "Engine.h"
#include "system/TickerNode.h"
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/context.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>

namespace Polaris {

using namespace godot;

// =============================================================================
// Class Registration
// =============================================================================

void PolarisEngine::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_debug_enabled", "enabled"), &PolarisEngine::set_debug_enabled);
    ClassDB::bind_method(D_METHOD("get_debug_enabled"), &PolarisEngine::get_debug_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_enabled"), "set_debug_enabled", "get_debug_enabled");

    ClassDB::bind_method(D_METHOD("get_entity_count"), &PolarisEngine::get_entity_count);
}

// =============================================================================
// Construction / Destruction
// =============================================================================

PolarisEngine::PolarisEngine() {
    singleton_instance = this;
    Log::info("[PolarisEngine] Created");
}

PolarisEngine::~PolarisEngine() {
    shutdown();

    if (singleton_instance == this) {
        singleton_instance = nullptr;
    }
}

// =============================================================================
// Lifecycle Management
// =============================================================================

void PolarisEngine::initialize() {
    Log::info("[PolarisEngine] Initializing subsystems...");

    // Create ECS context first
    m_ecs = memnew(Context::ECSWorld);
    godot::Engine::get_singleton()->register_singleton("ECSWorld", m_ecs);
    Log::print(m_debug_enabled, "[PolarisEngine] ECSWorld created");

    // Create node watcher
    m_watcher = memnew(System::NodeWatcher);
    godot::Engine::get_singleton()->register_singleton("NodeWatcher", m_watcher);
    Log::print(m_debug_enabled, "[PolarisEngine] NodeWatcher created");

    // Create ticker node (will be added to scene tree when available)
    m_ticker_node = memnew(System::TickerNode);
    godot::Engine::get_singleton()->register_singleton("PolarisTicker", m_ticker_node);
    m_ticker_node->initialize(&m_ecs->get_world());
    Log::print(m_debug_enabled, "[PolarisEngine] TickerNode created");

    // Wire callbacks
    m_watcher->set_on_node_added([this](Node* node, uint16_t depth, uint32_t tree_id) {
        _on_node_registered(node, depth, tree_id);

        // Add ticker node to scene tree once we have one
        if (m_ticker_node && !m_ticker_node->is_inside_tree()) {
            SceneTree* tree = m_watcher->get_scene_tree();
            if (tree && tree->get_root()) {
                tree->get_root()->add_child(m_ticker_node);
                Log::info("[PolarisEngine] TickerNode added to scene tree");
            }
        }
    });

    m_watcher->set_on_node_removed([this](Node* node) {
        _on_node_unregistered(node);
    });

    Log::print(m_debug_enabled, "[PolarisEngine] Callbacks wired");

    m_watcher->_try_auto_bind();

    Log::info("[PolarisEngine] Initialization complete, ", m_node_to_entity.size(), " entities created");
}

void PolarisEngine::shutdown() {
    if (m_shutting_down) return;  // Already shut down

    Log::info("[PolarisEngine] Shutting down...");

    // Set flag FIRST to skip all callbacks
    m_shutting_down = true;

    // Clear NodeWatcher callbacks immediately to prevent any more work
    if (m_watcher) {
        m_watcher->set_on_node_added(nullptr);
        m_watcher->set_on_node_removed(nullptr);
    }

    // Clear entity map - don't need to clean up individual entities
    m_node_to_entity.clear();

    // Unregister singletons first (order matters for dependencies)
    if (m_ticker_node) {
        godot::Engine::get_singleton()->unregister_singleton("PolarisTicker");
    }
    if (m_watcher) {
        godot::Engine::get_singleton()->unregister_singleton("NodeWatcher");
    }
    if (m_ecs) {
        godot::Engine::get_singleton()->unregister_singleton("ECSWorld");
    }

    // Now delete in reverse order
    if (m_ticker_node) {
        // Don't remove from tree - it causes more callbacks. Just delete.
        memdelete(m_ticker_node);
        m_ticker_node = nullptr;
    }

    if (m_watcher) {
        memdelete(m_watcher);
        m_watcher = nullptr;
    }

    if (m_ecs) {
        memdelete(m_ecs);
        m_ecs = nullptr;
    }

    Log::info("[PolarisEngine] Shutdown complete");
}

// =============================================================================
// Node <-> Entity Callbacks
// =============================================================================

void PolarisEngine::_on_node_registered(Node* node, uint16_t depth, uint32_t tree_id) {
    if (!node || !m_ecs) return;

    if (m_node_to_entity.find(node) != m_node_to_entity.end()) {
        Log::print(m_debug_enabled, "[PolarisEngine] Node already registered, skipping: ", node->get_name());
        return;
    }

    flecs::entity e = _create_entity_for_node(node, depth, tree_id);
    m_node_to_entity[node] = e;

    Log::print(m_debug_enabled, "[PolarisEngine] Registered: ", node->get_name(),
               " -> Entity ", e.id());

    // Notify context that ECS is ready for this node (if it has the method)
    Ref<godot::Context> ctx = node->get_context();
    if (ctx.is_valid()) {
        Log::print(m_debug_enabled, "[PolarisEngine] Node has context: ", node->get_name());
        if (ctx->has_method("_on_ecs_ready")) {
            Log::print(m_debug_enabled, "[PolarisEngine] Calling _on_ecs_ready for: ", node->get_name());
            ctx->call("_on_ecs_ready", node);
        } else {
            Log::print(m_debug_enabled, "[PolarisEngine] Context has no _on_ecs_ready method");
        }
    }

    if (m_debug_enabled) {
        m_ecs->print_state();
    }
}

void PolarisEngine::_on_node_unregistered(Node* node) {
    if (!node) return;

    // Skip cleanup during shutdown - world is being destroyed anyway
    if (m_shutting_down) return;

    Log::print(m_debug_enabled, "[PolarisEngine] Unregistering: ", node->get_name());

    // Notify context that ECS is about to remove this node (if it has the method)
    Ref<godot::Context> ctx = node->get_context();
    if (ctx.is_valid() && ctx->has_method("_on_ecs_exit")) {
        ctx->call("_on_ecs_exit", node);
    }

    _destroy_entity_for_node(node);

    if (m_debug_enabled && m_ecs) {
        m_ecs->print_state();
    }
}

// =============================================================================
// Entity Management
// =============================================================================

flecs::entity PolarisEngine::_create_entity_for_node(Node* node, uint16_t depth, uint32_t tree_id) {
    auto& world = m_ecs->get_world();

    // Use Godot's unique ObjectID for entity name
    // Format: "NodeName#12345" where 12345 is the ObjectID
    uint64_t obj_id = static_cast<uint64_t>(node->get_instance_id());
    String entity_name = String(node->get_name()) + "#" + String::num_uint64(obj_id);
    CharString name_utf8 = entity_name.utf8();

    flecs::entity e = world.entity(name_utf8.get_data());

    e.set<Component::GodotNode>(Component::GodotNode(node));
    e.set<Component::NodeDepth>({ .value = depth });
    e.set<Component::TreeId>({ .value = tree_id });
    e.add<Tag::Active>();

    _apply_class_tags(e, node);

    return e;
}

void PolarisEngine::_destroy_entity_for_node(Node* node) {
    auto it = m_node_to_entity.find(node);
    if (it == m_node_to_entity.end()) {
        return;
    }

    flecs::entity e = it->second;

    Log::print(m_debug_enabled, "[PolarisEngine] Destroying entity ", e.id(),
               " for node: ", node->get_name());

    if (e.is_valid() && e.is_alive()) {
        e.destruct();
    }

    m_node_to_entity.erase(it);
}

void PolarisEngine::_apply_class_tags(flecs::entity& e, Node* node) {
    auto& world = m_ecs->get_world();

    String current_class = node->get_class();

    while (!current_class.is_empty() && current_class != "Object") {
        CharString tag_utf8 = current_class.utf8();
        flecs::entity tag = world.entity(tag_utf8.get_data());
        e.add(tag);

        StringName parent_class = ClassDB::get_parent_class(current_class);
        if (parent_class == StringName()) break;
        current_class = String(parent_class);
    }
}

// =============================================================================
// World Access
// =============================================================================

flecs::world& PolarisEngine::get_world() noexcept {
    return m_ecs->get_world();
}

const flecs::world& PolarisEngine::get_world() const noexcept {
    return m_ecs->get_world();
}

// =============================================================================
// Node <-> Entity Lookups
// =============================================================================

flecs::entity PolarisEngine::get_entity_for_node(Node* node) const {
    auto it = m_node_to_entity.find(node);
    if (it != m_node_to_entity.end()) {
        return it->second;
    }
    return flecs::entity::null();
}

Node* PolarisEngine::get_node_for_entity(flecs::entity e) const {
    if (!e.is_valid() || !e.is_alive()) return nullptr;
    const Component::GodotNode* gn = e.try_get<Component::GodotNode>();
    return gn ? gn->get() : nullptr;
}

bool PolarisEngine::has_entity(Node* node) const {
    return m_node_to_entity.find(node) != m_node_to_entity.end();
}

// =============================================================================
// Singleton Management
// =============================================================================

PolarisEngine* PolarisEngine::create_global_instance() {
    // std::call_once guarantees thread-safe one-time initialization.
    //
    // WHY WE NEED THIS:
    // If two threads call create_global_instance() simultaneously without
    // protection, both might see singleton_instance as nullptr and both
    // would create instances - causing a memory leak and undefined behavior.
    //
    // std::call_once uses an internal mutex to ensure the lambda runs
    // exactly once, even under concurrent access. The std::once_flag
    // tracks whether initialization has completed.
    //
    static std::once_flag init_once;

    std::call_once(init_once, []() {
        singleton_instance = memnew(PolarisEngine);
        godot::Engine::get_singleton()->register_singleton("Polaris", singleton_instance);
        singleton_instance->initialize();
    });

    return singleton_instance;
}

void PolarisEngine::destroy_global_instance() {
    if (singleton_instance) {
        godot::Engine::get_singleton()->unregister_singleton("Polaris");
        memdelete(singleton_instance);
        singleton_instance = nullptr;
    }
}

} // namespace Polaris
