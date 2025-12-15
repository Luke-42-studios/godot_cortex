#ifndef CPOLARIS_H
#define CPOLARIS_H

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/classes/node.hpp>
#include <flecs.h>
#include <unordered_map>
#include <mutex>

#include "system/PECSContext.h"

using namespace godot;

// Forward declarations
class SNodeWatcher;
struct SNodeData;

// =============================================================================
// CPolaris - Framework Coordinator
// Owns and orchestrates all Polaris subsystems
// =============================================================================
class CPolaris : public Object {
    GDCLASS(CPolaris, Object)

private:
    static inline CPolaris* singleton_instance = nullptr;

    // Owned subsystems
    PECSContext* m_ecs = nullptr;
    SNodeWatcher* m_watcher = nullptr;

    // Node <-> Entity bidirectional mapping
    std::unordered_map<Node*, flecs::entity> m_node_to_entity;
    std::unordered_map<uint64_t, Node*> m_entity_to_node;  // entity.id() -> Node*

    // Prevent copying (operator= already handled by GDCLASS)
    CPolaris(const CPolaris&) = delete;

    // Internal callbacks (wired to SNodeWatcher)
    void _on_node_registered(Node* node, const SNodeData& data);
    void _on_node_unregistered(Node* node);

    // Entity management
    flecs::entity _create_entity_for_node(Node* node, const SNodeData& data);
    void _destroy_entity_for_node(Node* node);

    // Add class hierarchy as ECS tags
    void _apply_class_tags(flecs::entity& e, const std::vector<String>& components);

protected:
    static void _bind_methods();

public:
    CPolaris();
    ~CPolaris();

    // Singleton access
    static CPolaris* get_singleton() noexcept { return singleton_instance; }

    // Subsystem access (read-only to maintain encapsulation)
    PECSContext* get_ecs() const noexcept { return m_ecs; }
    SNodeWatcher* get_watcher() const noexcept { return m_watcher; }

    // Direct world access for convenience
    flecs::world& get_world() noexcept;
    const flecs::world& get_world() const noexcept;

    // Node <-> Entity lookups
    flecs::entity get_entity_for_node(Node* node) const;
    Node* get_node_for_entity(flecs::entity e) const;
    Node* get_node_for_entity_id(uint64_t entity_id) const;
    bool has_entity(Node* node) const;

    // Stats
    size_t get_entity_count() const { return m_node_to_entity.size(); }

    // Lifecycle
    void initialize();   // Called after construction, wires everything up
    void shutdown();     // Cleanup before destruction

    // Module integration
    static CPolaris* create_global_instance();
    static void destroy_global_instance();
};

#endif // CPOLARIS_H
