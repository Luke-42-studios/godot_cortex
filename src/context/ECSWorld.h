#ifndef POLARIS_CONTEXT_ECS_WORLD_H
#define POLARIS_CONTEXT_ECS_WORLD_H

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <flecs.h>
#include <memory>
#include <mutex>

#include "../component/GodotNode.h"
#include "../tag/Tags.h"
#include "../Log.h"

namespace Polaris {
namespace Context {

using namespace godot;

/// ECSWorld - Flecs World Manager
///
/// Single source of truth for all ECS data. Provides:
/// - Flecs world ownership and lifecycle
/// - Component registration
/// - Pre-built queries for common operations
/// - Safe node access helpers
///
class ECSWorld : public Object {
    GDCLASS(ECSWorld, Object)

private:
    static inline ECSWorld* singleton_instance = nullptr;

    std::unique_ptr<flecs::world> m_world;

    // Pre-built queries
    flecs::query<Component::GodotNode> m_all_nodes_query;
    flecs::query<Component::GodotNode, Component::NodeDepth> m_nodes_with_depth_query;

    // Debug flag - controllable from GDScript
    bool m_debug_enabled = false;

    // Prevent copying
    ECSWorld(const ECSWorld&) = delete;

protected:
    static void _bind_methods();

public:
    ECSWorld();
    ~ECSWorld();

    // =========================================================================
    // Debug Control
    // =========================================================================

    void set_debug_enabled(bool enabled) { m_debug_enabled = enabled; }
    bool get_debug_enabled() const { return m_debug_enabled; }

    // =========================================================================
    // World Access
    // =========================================================================

    [[nodiscard]] flecs::world& get_world() noexcept { return *m_world; }
    [[nodiscard]] const flecs::world& get_world() const noexcept { return *m_world; }

    // =========================================================================
    // Safe Node Access
    // =========================================================================

    [[nodiscard]] Node* get_node(flecs::entity e) const;

    template<typename T>
    [[nodiscard]] T* get_node_as(flecs::entity e) const {
        const Component::GodotNode* gn = e.try_get<Component::GodotNode>();
        return gn ? gn->get_as<T>() : nullptr;
    }

    [[nodiscard]] bool is_node_valid(flecs::entity e) const;

    // =========================================================================
    // Query Helpers
    // =========================================================================

    [[nodiscard]] flecs::entity find_entity_by_node(Node* node) const;
    [[nodiscard]] flecs::entity find_entity_by_id(ObjectID id) const;

    template<typename Func>
    void each_valid_node(Func&& fn) {
        m_all_nodes_query.each([&](flecs::entity e, Component::GodotNode& gn) {
            if (Node* node = gn.get_safe()) {
                fn(e, node);
            }
        });
    }

    template<typename Func>
    void each_at_depth(uint16_t depth, Func&& fn) {
        m_nodes_with_depth_query.each([&](flecs::entity e, Component::GodotNode& gn, Component::NodeDepth& d) {
            if (d.value == depth) {
                if (Node* node = gn.get_safe()) {
                    fn(e, node);
                }
            }
        });
    }

    template<typename Func>
    void each_with_class(const char* class_name, Func&& fn) {
        flecs::entity tag = m_world->lookup(class_name);
        if (!tag.is_valid()) return;

        m_world->query_builder<Component::GodotNode>()
            .with(tag)
            .build()
            .each([&](flecs::entity e, Component::GodotNode& gn) {
                if (Node* node = gn.get_safe()) {
                    fn(e, node);
                }
            });
    }

    [[nodiscard]] size_t count_with_class(const char* class_name) const;
    [[nodiscard]] size_t get_entity_count() const;

    // =========================================================================
    // Batch Operations
    // =========================================================================

    size_t validate_all_caches();

    // =========================================================================
    // Debug
    // =========================================================================

    void print_state() const;

    // =========================================================================
    // Singleton
    // =========================================================================

    static ECSWorld* get_singleton() noexcept { return singleton_instance; }
    static ECSWorld* create_global_instance();
    static void destroy_global_instance();

private:
    void _initialize();
    void _shutdown();
};

} // namespace Context
} // namespace Polaris

#endif // POLARIS_CONTEXT_ECS_WORLD_H
