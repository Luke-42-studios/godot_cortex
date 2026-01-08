#ifndef POLARIS_COMPONENTS_GD_H
#define POLARIS_COMPONENTS_GD_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/object.hpp>

namespace Polaris {

using namespace godot;

// =============================================================================
// Gd:: Components - Pointers to Godot nodes (the "view")
// =============================================================================
//
// These components hold references to Godot nodes, bridging ECS data with
// the visual scene tree. Systems can read/write Godot node state through these.
//
// Pattern: All Gd:: components should implement:
//   - static T create(Node* root)  - Factory method
//   - bool is_valid() const        - Check pointer before use
//   - void unbind()                - Set pointer to nullptr (call in decompose)
//
// =============================================================================

namespace Gd {

// -----------------------------------------------------------------------------
// Node - Base root node reference (generic)
// -----------------------------------------------------------------------------
// Use when you only need basic node access without transform helpers.

struct Node {
    godot::Node* root = nullptr;

    static Node create(godot::Node* n) { return { n }; }

    bool is_valid() const { return root != nullptr; }
    void unbind() { root = nullptr; }

    // Basic node access
    String get_name() const { return root ? root->get_name() : String(); }
    godot::Node* get_root() const { return root; }
};

// -----------------------------------------------------------------------------
// Node3D - Root node reference with 3D transform helpers
// -----------------------------------------------------------------------------
// Use for 3D entities that need position/rotation access.

struct Node3D {
    godot::Node3D* root = nullptr;

    static Node3D create(godot::Node* n) {
        return { Object::cast_to<godot::Node3D>(n) };
    }

    bool is_valid() const { return root != nullptr; }
    void unbind() { root = nullptr; }

    // Transform helpers
    Vector3 get_position() const { return root ? root->get_global_position() : Vector3(); }
    void set_position(const Vector3& p) { if (root) root->set_global_position(p); }

    Quaternion get_rotation() const { return root ? root->get_quaternion() : Quaternion(); }
    void set_rotation(const Quaternion& q) { if (root) root->set_quaternion(q); }

    Basis get_basis() const { return root ? root->get_global_basis() : Basis(); }
    Vector3 get_forward() const { return root ? -root->get_global_basis().get_column(2) : Vector3(0, 0, -1); }
    Vector3 get_right() const { return root ? root->get_global_basis().get_column(0) : Vector3(1, 0, 0); }
    Vector3 get_up() const { return root ? root->get_global_basis().get_column(1) : Vector3(0, 1, 0); }
};

// -----------------------------------------------------------------------------
// Node2D - Root node reference with 2D transform helpers
// -----------------------------------------------------------------------------
// Use for 2D entities that need position/rotation access.

struct Node2D {
    godot::Node2D* root = nullptr;

    static Node2D create(godot::Node* n) {
        return { Object::cast_to<godot::Node2D>(n) };
    }

    bool is_valid() const { return root != nullptr; }
    void unbind() { root = nullptr; }

    // Transform helpers
    Vector2 get_position() const { return root ? root->get_global_position() : Vector2(); }
    void set_position(const Vector2& p) { if (root) root->set_global_position(p); }

    float get_rotation() const { return root ? root->get_rotation() : 0.0f; }
    void set_rotation(float r) { if (root) root->set_rotation(r); }
};

} // namespace Gd

} // namespace Polaris

#endif // POLARIS_COMPONENTS_GD_H
