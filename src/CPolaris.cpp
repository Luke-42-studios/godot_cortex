// CPolaris.cpp - Framework Coordinator Implementation
#include "CPolaris.h"
#include "system/PECSContext.h"
#include "system/SNodeWatcher.h"

#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

// =============================================================================
// Class Registration
// =============================================================================

void CPolaris::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_entity_count"), &CPolaris::get_entity_count);
}

// =============================================================================
// Construction / Destruction
// =============================================================================

CPolaris::CPolaris() {
    singleton_instance = this;
    UtilityFunctions::print("[CPolaris] Coordinator created");
}

CPolaris::~CPolaris() {
    shutdown();

    if (singleton_instance == this) {
        singleton_instance = nullptr;
    }
}

// =============================================================================
// Lifecycle Management
// =============================================================================

void CPolaris::initialize() {
    UtilityFunctions::print("[CPolaris] Initializing subsystems...");

    // Create ECS context first (no dependencies)
    m_ecs = memnew(PECSContext);
    Engine::get_singleton()->register_singleton("FlecsWorld", m_ecs);
    UtilityFunctions::print("[CPolaris] PECSContext created");

    // Create node watcher
    m_watcher = memnew(SNodeWatcher);
    Engine::get_singleton()->register_singleton("SNodeWatcher", m_watcher);
    UtilityFunctions::print("[CPolaris] SNodeWatcher created");

    // Wire callbacks - this is where the magic happens
    // SNodeWatcher doesn't know about ECS, CPolaris bridges them
    m_watcher->set_on_node_added([this](Node* node, const SNodeData& data) {
        _on_node_registered(node, data);
    });

    m_watcher->set_on_node_removed([this](Node* node) {
        _on_node_unregistered(node);
    });

    UtilityFunctions::print("[CPolaris] Callbacks wired");

    // Trigger auto-bind which will collect existing nodes and fire callbacks
    m_watcher->_try_auto_bind();

    UtilityFunctions::print("[CPolaris] Initialization complete, ",
                            m_node_to_entity.size(), " entities created");

    // Print ECS world state
    m_ecs->print_state();
}

void CPolaris::shutdown() {
    UtilityFunctions::print("[CPolaris] Shutting down...");

    // Clear mappings first
    m_node_to_entity.clear();
    m_entity_to_node.clear();

    // Destroy subsystems in reverse order
    if (m_watcher) {
        Engine::get_singleton()->unregister_singleton("SNodeWatcher");
        memdelete(m_watcher);
        m_watcher = nullptr;
    }

    if (m_ecs) {
        Engine::get_singleton()->unregister_singleton("FlecsWorld");
        memdelete(m_ecs);
        m_ecs = nullptr;
    }

    UtilityFunctions::print("[CPolaris] Shutdown complete");
}

// =============================================================================
// Node <-> Entity Callbacks
// =============================================================================

void CPolaris::_on_node_registered(Node* node, const SNodeData& data) {
    UtilityFunctions::print("[CPolaris] _on_node_registered called");

    if (!node) {
        UtilityFunctions::print("[CPolaris] ERROR: node is null");
        return;
    }
    if (!m_ecs) {
        UtilityFunctions::print("[CPolaris] ERROR: m_ecs is null");
        return;
    }

    String node_name = String(node->get_name());
    UtilityFunctions::print("[CPolaris] Registering node: ", node_name);

    // Skip if already registered (safety check)
    if (m_node_to_entity.find(node) != m_node_to_entity.end()) {
        UtilityFunctions::print("[CPolaris] Node already registered, skipping");
        return;
    }

    flecs::entity e = _create_entity_for_node(node, data);
    UtilityFunctions::print("[CPolaris] Created entity: ", e.id(), " for node: ", node_name);

    // Store bidirectional mapping
    m_node_to_entity[node] = e;
    m_entity_to_node[e.id()] = node;

    UtilityFunctions::print("[CPolaris] Total entities now: ", m_node_to_entity.size());

    // Print updated ECS state
    m_ecs->print_state();
}

void CPolaris::_on_node_unregistered(Node* node) {
    if (!node) return;
    _destroy_entity_for_node(node);

    // Print updated ECS state
    m_ecs->print_state();
}

// =============================================================================
// Entity Management
// =============================================================================

flecs::entity CPolaris::_create_entity_for_node(Node* node, const SNodeData& data) {
    auto& world = m_ecs->get_world();

    // Create entity with node name for debugging
    String name_str = String(node->get_name());
    CharString name_utf8 = name_str.utf8();
    flecs::entity e = world.entity(name_utf8.get_data());

    // Set core component with node reference
    e.set<CGodotNode>({
        .ptr = node,
        .instance_id = node->get_instance_id()
    });

    // Set depth component
    e.set<CNodeDepth>({ .value = data.depth });

    // Set tree ID component
    e.set<CTreeId>({ .value = data.tree_id });

    // Apply class hierarchy as tags
    _apply_class_tags(e, data.components);

    return e;
}

void CPolaris::_destroy_entity_for_node(Node* node) {
    auto it = m_node_to_entity.find(node);
    if (it == m_node_to_entity.end()) {
        return;  // Not tracked
    }

    flecs::entity e = it->second;
    uint64_t entity_id = e.id();

    // Destroy the ECS entity
    if (e.is_valid() && e.is_alive()) {
        e.destruct();
    }

    // Remove from mappings
    m_node_to_entity.erase(it);
    m_entity_to_node.erase(entity_id);
}

void CPolaris::_apply_class_tags(flecs::entity& e, const std::vector<String>& components) {
    auto& world = m_ecs->get_world();

    // Each class in the hierarchy becomes a tag
    // e.g., MeshInstance3D -> GeometryInstance3D -> VisualInstance3D -> Node3D -> Node
    for (const String& class_name : components) {
        CharString tag_utf8 = class_name.utf8();

        // Get or create a tag entity for this class name
        flecs::entity tag = world.entity(tag_utf8.get_data());

        // Add the tag to our node entity
        e.add(tag);
    }
}

// =============================================================================
// World Access
// =============================================================================

flecs::world& CPolaris::get_world() noexcept {
    return m_ecs->get_world();
}

const flecs::world& CPolaris::get_world() const noexcept {
    return m_ecs->get_world();
}

// =============================================================================
// Node <-> Entity Lookups
// =============================================================================

flecs::entity CPolaris::get_entity_for_node(Node* node) const {
    auto it = m_node_to_entity.find(node);
    if (it != m_node_to_entity.end()) {
        return it->second;
    }
    return flecs::entity::null();
}

Node* CPolaris::get_node_for_entity(flecs::entity e) const {
    return get_node_for_entity_id(e.id());
}

Node* CPolaris::get_node_for_entity_id(uint64_t entity_id) const {
    auto it = m_entity_to_node.find(entity_id);
    if (it != m_entity_to_node.end()) {
        return it->second;
    }
    return nullptr;
}

bool CPolaris::has_entity(Node* node) const {
    return m_node_to_entity.find(node) != m_node_to_entity.end();
}

// =============================================================================
// Singleton Management
// =============================================================================

CPolaris* CPolaris::create_global_instance() {
    static std::once_flag init_once;

    std::call_once(init_once, []() {
        singleton_instance = memnew(CPolaris);
        Engine::get_singleton()->register_singleton("Polaris", singleton_instance);

        // Initialize after registration
        singleton_instance->initialize();
    });

    return singleton_instance;
}

void CPolaris::destroy_global_instance() {
    if (singleton_instance) {
        Engine::get_singleton()->unregister_singleton("Polaris");
        memdelete(singleton_instance);
        singleton_instance = nullptr;
    }
}
