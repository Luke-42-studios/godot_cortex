#ifndef POLARIS_COMPOSITION_FACTORY_H
#define POLARIS_COMPOSITION_FACTORY_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "composition/Composition.h"

namespace Polaris {

using namespace godot;

// =============================================================================
// CompositionFactory - Watches SceneTree for nodes with compositions
// =============================================================================
//
// Monitors the scene tree for nodes that have "composition" metadata.
// When detected, creates a flecs entity and calls composition->compose().
// When removed, calls decompose() and destroys the entity.
//
// USAGE:
//   Add to scene as part of Polaris autoload (sibling to PipelineNode).
//   Factory automatically binds to SceneTree on _ready().
//
// =============================================================================

class CompositionFactory : public Node {
    GDCLASS(CompositionFactory, Node)

private:
    SceneTree* m_scene_tree = nullptr;
    bool m_debug_enabled = false;
    int m_entity_count = 0;

protected:
    static void _bind_methods();

public:
    CompositionFactory();
    ~CompositionFactory();

    // =========================================================================
    // Godot Lifecycle
    // =========================================================================

    void _ready() override;
    void _exit_tree() override;

    // =========================================================================
    // Scene Tree Binding
    // =========================================================================

    void bind_to_scene_tree(SceneTree* tree);
    void unbind_from_scene_tree();

    // =========================================================================
    // Signal Handlers (bound to SceneTree signals)
    // =========================================================================

    void _on_node_added(Node* node);
    void _on_node_removed(Node* node);
    void _try_auto_bind();

    // =========================================================================
    // Properties
    // =========================================================================

    void set_debug_enabled(bool enabled) { m_debug_enabled = enabled; }
    bool get_debug_enabled() const { return m_debug_enabled; }

    int get_entity_count() const { return m_entity_count; }

private:
    void _collect_existing_nodes(Node* root);
};

} // namespace Polaris

#endif // POLARIS_COMPOSITION_FACTORY_H
