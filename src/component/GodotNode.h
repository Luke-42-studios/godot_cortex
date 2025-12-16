#ifndef POLARIS_COMPONENT_GODOT_NODE_H
#define POLARIS_COMPONENT_GODOT_NODE_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/object.hpp>

namespace Polaris {
namespace Component {

using namespace godot;

// =============================================================================
// Components - Pure data attached to entities
// =============================================================================

/// GodotNode - Hybrid safe reference to a Godot Node
///
/// Uses ObjectID for safety with cached pointer for performance.
/// The cached pointer is validated on access and updated if stale.
///
/// Memory layout: 16 bytes (same as two pointers)
///
/// USAGE:
///   Node* node = component.get();           // Safe path (validates via ObjectDB)
///   Node* node = component.get_unchecked(); // Fast path (uses cache)
///   if (component.is_valid()) { ... }       // Check validity
///
struct GodotNode {
    ObjectID id;                    // 8 bytes - Godot's safe object identifier
    mutable Node* cached_ptr;       // 8 bytes - Cached pointer (mutable for lazy update)

    // Default constructor
    GodotNode() : id(), cached_ptr(nullptr) {}

    // Construct from Node pointer
    explicit GodotNode(Node* node)
        : id(node ? node->get_instance_id() : ObjectID())
        , cached_ptr(node) {}

    // Construct from ObjectID only
    explicit GodotNode(ObjectID obj_id)
        : id(obj_id)
        , cached_ptr(nullptr) {}

    /// Fast access - returns cached pointer without validation
    /// WARNING: May return dangling pointer if node was freed
    [[nodiscard]] Node* get_unchecked() const noexcept {
        return cached_ptr;
    }

    /// Safe access - validates via Godot's ObjectDB and updates cache
    /// Returns nullptr if node was freed
    [[nodiscard]] Node* get_safe() const {
        Object* obj = ObjectDB::get_instance(id);
        Node* valid_ptr = Object::cast_to<Node>(obj);
        cached_ptr = valid_ptr;
        return valid_ptr;
    }

    /// Default accessor - uses safe path
    [[nodiscard]] Node* get() const {
        return get_safe();
    }

    /// Type-safe accessor for any node type
    template<typename T>
    [[nodiscard]] T* get_as() const {
        return Object::cast_to<T>(get_safe());
    }

    /// Check if the referenced node still exists
    [[nodiscard]] bool is_valid() const {
        return get_safe() != nullptr;
    }

    /// Check if this component has been initialized
    [[nodiscard]] bool is_null() const noexcept {
        return id == ObjectID();
    }

    /// Get the raw ObjectID
    [[nodiscard]] ObjectID get_id() const noexcept {
        return id;
    }

    /// Validate cache and return validity status
    [[nodiscard]] bool validate_cache() const {
        return get_safe() != nullptr;
    }
};

/// NodeDepth - Hierarchy depth in scene tree
struct NodeDepth {
    uint16_t value = 0;
};

/// TreeId - Scene tree identifier for multi-tree support
struct TreeId {
    uint32_t value = 0;
};

} // namespace Component
} // namespace Polaris

#endif // POLARIS_COMPONENT_GODOT_NODE_H
