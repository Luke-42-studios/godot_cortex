#include "ECSWorld.h"
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/classes/engine.hpp>

namespace Polaris {
namespace Context {

using namespace godot;

// =============================================================================
// Class Registration
// =============================================================================

void ECSWorld::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_debug_enabled", "enabled"), &ECSWorld::set_debug_enabled);
    ClassDB::bind_method(D_METHOD("get_debug_enabled"), &ECSWorld::get_debug_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_enabled"), "set_debug_enabled", "get_debug_enabled");

    ClassDB::bind_method(D_METHOD("print_state"), &ECSWorld::print_state);
    ClassDB::bind_method(D_METHOD("get_entity_count"), &ECSWorld::get_entity_count);
}

// =============================================================================
// Construction / Destruction
// =============================================================================

ECSWorld::ECSWorld()
    : m_world(std::make_unique<flecs::world>()) {
    singleton_instance = this;
    _initialize();
    Log::info("[Polaris::Context::ECSWorld] Created");
}

ECSWorld::~ECSWorld() {
    _shutdown();
    if (singleton_instance == this) {
        singleton_instance = nullptr;
    }
}

void ECSWorld::_initialize() {
    auto& world = *m_world;
    world.set_target_fps(60.0f);

    // Register components
    world.component<Component::GodotNode>()
        .member<ObjectID>("id");
    world.component<Component::NodeDepth>()
        .member<uint16_t>("value");
    world.component<Component::TreeId>()
        .member<uint32_t>("value");

    // Register tags
    world.component<Tag::Active>();

    // Pre-build queries
    m_all_nodes_query = world.query<Component::GodotNode>();
    m_nodes_with_depth_query = world.query<Component::GodotNode, Component::NodeDepth>();

    Log::info("[Polaris::Context::ECSWorld] Components registered");
}

void ECSWorld::_shutdown() {
    Log::print(m_debug_enabled, "[Polaris::Context::ECSWorld] Shutting down...");
    if (m_world) {
        m_world->quit();
    }
}

// =============================================================================
// Safe Node Access
// =============================================================================

Node* ECSWorld::get_node(flecs::entity e) const {
    if (!e.is_valid() || !e.is_alive()) return nullptr;
    const Component::GodotNode* gn = e.try_get<Component::GodotNode>();
    return gn ? gn->get_safe() : nullptr;
}

bool ECSWorld::is_node_valid(flecs::entity e) const {
    if (!e.is_valid() || !e.is_alive()) return false;
    const Component::GodotNode* gn = e.try_get<Component::GodotNode>();
    return gn && gn->is_valid();
}

// =============================================================================
// Query Helpers
// =============================================================================

flecs::entity ECSWorld::find_entity_by_node(Node* node) const {
    if (!node) return flecs::entity::null();

    auto target_id = node->get_instance_id();
    flecs::entity result = flecs::entity::null();

    m_all_nodes_query.each([&](flecs::entity e, Component::GodotNode& gn) {
        if (static_cast<uint64_t>(gn.get_id()) == target_id) {
            result = e;
        }
    });

    return result;
}

flecs::entity ECSWorld::find_entity_by_id(ObjectID id) const {
    flecs::entity result = flecs::entity::null();

    m_all_nodes_query.each([&](flecs::entity e, Component::GodotNode& gn) {
        if (gn.get_id() == id) {
            result = e;
        }
    });

    return result;
}

size_t ECSWorld::count_with_class(const char* class_name) const {
    flecs::entity tag = m_world->lookup(class_name);
    if (!tag.is_valid()) return 0;

    size_t count = 0;
    m_world->query_builder<Component::GodotNode>()
        .with(tag)
        .build()
        .each([&](flecs::entity e, Component::GodotNode& gn) {
            count++;
        });

    return count;
}

size_t ECSWorld::get_entity_count() const {
    size_t count = 0;
    m_all_nodes_query.each([&](flecs::entity e, Component::GodotNode& gn) {
        count++;
    });
    return count;
}

// =============================================================================
// Batch Operations
// =============================================================================

size_t ECSWorld::validate_all_caches() {
    size_t invalid_count = 0;

    m_all_nodes_query.each([&](flecs::entity e, Component::GodotNode& gn) {
        if (!gn.validate_cache()) {
            invalid_count++;
        }
    });

    Log::print(m_debug_enabled, "[Polaris::Context::ECSWorld] Validated caches, found ",
               invalid_count, " invalid");

    return invalid_count;
}

// =============================================================================
// Debug
// =============================================================================

void ECSWorld::print_state() const {
    Log::info("[Polaris::Context::ECSWorld] ==========================================");
    Log::info("[Polaris::Context::ECSWorld] Flecs World State");
    Log::info("[Polaris::Context::ECSWorld] ==========================================");

    if (!m_world) {
        Log::info("[Polaris::Context::ECSWorld] <world not initialized>");
        return;
    }

    int entity_count = 0;
    int valid_count = 0;

    m_all_nodes_query.each([&](flecs::entity e, Component::GodotNode& gn) {
        entity_count++;

        const char* name = e.name().c_str();
        String entity_name = (name && name[0] != '\0') ? String(name) : String("<unnamed>");

        Node* node = gn.get_safe();
        String status = node ? "VALID" : "INVALID";
        String node_class = node ? String(node->get_class()) : String("?");

        if (node) valid_count++;

        Log::info("[Polaris::Context::ECSWorld] Entity ", e.id(), ": ", entity_name,
                  " [", node_class, "] (", status, ")");

        e.each([](flecs::id id) {
            if (id.is_entity()) {
                flecs::entity comp = id.entity();
                const char* comp_name = comp.name().c_str();
                if (comp_name && comp_name[0] != '\0') {
                    Log::info("[Polaris::Context::ECSWorld]   - ", comp_name);
                }
            }
        });
    });

    Log::info("[Polaris::Context::ECSWorld] ------------------------------------------");
    Log::info("[Polaris::Context::ECSWorld] Total entities: ", entity_count);
    Log::info("[Polaris::Context::ECSWorld] Valid nodes: ", valid_count);
    Log::info("[Polaris::Context::ECSWorld] ==========================================");
}

// =============================================================================
// Singleton Management
// =============================================================================

ECSWorld* ECSWorld::create_global_instance() {
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
        singleton_instance = memnew(ECSWorld);
        godot::Engine::get_singleton()->register_singleton("ECSWorld", singleton_instance);
    });

    return singleton_instance;
}

void ECSWorld::destroy_global_instance() {
    if (singleton_instance) {
        godot::Engine::get_singleton()->unregister_singleton("ECSWorld");
        memdelete(singleton_instance);
        singleton_instance = nullptr;
    }
}

} // namespace Context
} // namespace Polaris
