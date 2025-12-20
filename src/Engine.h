#ifndef POLARIS_ENGINE_H
#define POLARIS_ENGINE_H

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <flecs.h>
#include <unordered_map>
#include <mutex>

#include "component/GodotNode.h"
#include "tag/Tags.h"
#include "Log.h"
#include "context/ECSWorld.h"
#include "system/NodeWatcher.h"
#include "system/FrameTicker.h"

namespace Polaris {

using namespace godot;

/// PolarisEngine - Main Polaris Coordinator
///
/// Owns and orchestrates all Polaris subsystems. Acts as the bridge between
/// Godot's scene tree and the ECS world.
///
/// Maintains a single lookup map (Node* -> entity) for fast access.
/// Reverse lookup (entity -> Node*) is done via ECS component access.
///
class PolarisEngine : public Object {
    GDCLASS(PolarisEngine, Object)

private:
    static inline PolarisEngine* singleton_instance = nullptr;

    Context::ECSWorld* m_ecs = nullptr;
    System::NodeWatcher* m_watcher = nullptr;
    System::FrameTicker* m_ticker = nullptr;

    std::unordered_map<Node*, flecs::entity> m_node_to_entity;

    bool m_debug_enabled = false;

    PolarisEngine(const PolarisEngine&) = delete;

protected:
    static void _bind_methods();

public:
    PolarisEngine();
    ~PolarisEngine();

    // =========================================================================
    // Debug Control
    // =========================================================================

    void set_debug_enabled(bool enabled) { m_debug_enabled = enabled; }
    bool get_debug_enabled() const { return m_debug_enabled; }

    // =========================================================================
    // Lifecycle
    // =========================================================================

    void initialize();
    void shutdown();

    // =========================================================================
    // Subsystem Access
    // =========================================================================

    [[nodiscard]] Context::ECSWorld* get_ecs() const noexcept { return m_ecs; }
    [[nodiscard]] System::NodeWatcher* get_watcher() const noexcept { return m_watcher; }
    [[nodiscard]] System::FrameTicker* get_ticker() const noexcept { return m_ticker; }
    [[nodiscard]] flecs::world& get_world() noexcept;
    [[nodiscard]] const flecs::world& get_world() const noexcept;

    // =========================================================================
    // Node <-> Entity Lookups
    // =========================================================================

    [[nodiscard]] flecs::entity get_entity_for_node(Node* node) const;
    [[nodiscard]] Node* get_node_for_entity(flecs::entity e) const;
    [[nodiscard]] bool has_entity(Node* node) const;
    [[nodiscard]] size_t get_entity_count() const { return m_node_to_entity.size(); }

    // =========================================================================
    // Singleton
    // =========================================================================

    static PolarisEngine* get_singleton() noexcept { return singleton_instance; }
    static PolarisEngine* create_global_instance();
    static void destroy_global_instance();

private:
    void _on_node_registered(Node* node, uint16_t depth, uint32_t tree_id);
    void _on_node_unregistered(Node* node);

    flecs::entity _create_entity_for_node(Node* node, uint16_t depth, uint32_t tree_id);
    void _destroy_entity_for_node(Node* node);
    void _apply_class_tags(flecs::entity& e, Node* node);
};

} // namespace Polaris

#endif // POLARIS_ENGINE_H
