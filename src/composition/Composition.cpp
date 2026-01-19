#include "Composition.h"
#include "util/Log.h"

namespace Polaris {

// =============================================================================
// Class Registration
// =============================================================================

void Composition::_bind_methods() {
    // Base class has no exposed properties
    // Subclasses should override _bind_methods() to expose their properties
}

// =============================================================================
// Entity Lifecycle
// =============================================================================

void Composition::compose(flecs::entity entity, Node* root) {
    // Store entity for sync<T>() to work
    m_entity = entity;

    // Always set base Gd::Node for generic node access
    entity.set<Gd::Node>(Gd::Node::create(root));

    // Also set Gd::Node3D if root is a 3D node
    if (Object::cast_to<godot::Node3D>(root)) {
        entity.set<Gd::Node3D>(Gd::Node3D::create(root));
        Log::info("[Composition] Set Gd::Node3D for entity #", entity.id());
    }

    // Subclasses override to add additional components
    // 2D compositions should set Gd::Node2D themselves
}

void Composition::decompose(flecs::entity entity) {
    // Unbind node references to prevent dangling pointers
    if (auto* node = entity.try_get_mut<Gd::Node>()) {
        node->unbind();
    }
    if (auto* node3d = entity.try_get_mut<Gd::Node3D>()) {
        node3d->unbind();
    }

    // Clear entity reference
    m_entity = flecs::entity();

    // Subclasses override to cleanup additional components
}

} // namespace Polaris
