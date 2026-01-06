#ifndef POLARIS_COMPOSITION_H
#define POLARIS_COMPOSITION_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <flecs.h>

#include "components/Gd.h"

namespace Polaris {

using namespace godot;

// =============================================================================
// Composition - Base class for entity archetypes
// =============================================================================
//
// A Composition is a Godot Resource that defines what components an entity
// has. When a node with a "composition" metadata enters the scene tree,
// CompositionFactory creates an entity and calls compose() to set up components.
//
// USAGE:
//   1. Subclass Composition (e.g., PlayerPawn, EnemyPawn)
//   2. Override compose() to add components
//   3. Create .tres resource in editor
//   4. Attach to node via metadata: node.set_meta("composition", resource)
//
// =============================================================================

class Composition : public Resource {
    GDCLASS(Composition, Resource)

protected:
    static void _bind_methods();

public:
    Composition() = default;
    virtual ~Composition() = default;

    // =========================================================================
    // Entity Lifecycle
    // =========================================================================

    /// Called when entity is created - add components and reference nodes
    /// Base implementation sets Gd::Node for 3D nodes
    /// @param entity The flecs entity to compose
    /// @param root The Godot node that triggered composition
    virtual void compose(flecs::entity entity, Node* root);

    /// Called when entity is destroyed - cleanup (optional override)
    /// Base implementation nulls out Gd::Node
    /// @param entity The flecs entity being decomposed
    virtual void decompose(flecs::entity entity);
};

} // namespace Polaris

#endif // POLARIS_COMPOSITION_H
