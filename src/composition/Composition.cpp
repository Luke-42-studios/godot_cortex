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
    // Set root node reference for 3D nodes
    if (auto* root3d = Object::cast_to<Node3D>(root)) {
        entity.set<Gd::Node>({ .root = root3d });
        Log::info("[Composition] Set Gd::Node for entity #", entity.id());
    }

    // Subclasses override to add additional components
    // 2D compositions should set Gd::Node2D themselves
}

void Composition::decompose(flecs::entity entity) {
    // Null out node reference to prevent dangling pointer
    Gd::Node* node = entity.try_get_mut<Gd::Node>();
    if (node) {
        node->root = nullptr;
    }

    // Subclasses override to cleanup additional components
}

} // namespace Polaris
