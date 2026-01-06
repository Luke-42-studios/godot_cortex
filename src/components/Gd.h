#ifndef POLARIS_COMPONENTS_GD_H
#define POLARIS_COMPONENTS_GD_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/node2d.hpp>

namespace Polaris {

using namespace godot;

// =============================================================================
// Gd:: Components - Pointers to Godot nodes (the "view")
// =============================================================================
//
// These components hold references to Godot nodes, bridging ECS data with
// the visual scene tree. Systems can read/write Godot node state through these.
//
// IMPORTANT: Always null-check before use, and null out in decompose()
//
// =============================================================================

namespace Gd {

// -----------------------------------------------------------------------------
// Node - Root node reference with transform helpers (3D)
// -----------------------------------------------------------------------------
// Automatically set by Composition::compose() for 3D entities.
// Provides convenient transform access.

struct Node {
    Node3D* root = nullptr;

    bool valid() const { return root != nullptr; }

    // Transform helpers (check valid() first)
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
// Node2D - Root node reference for 2D entities
// -----------------------------------------------------------------------------
// Set by 2D-specific compositions that override compose()

struct Node2D {
    godot::Node2D* root = nullptr;

    bool valid() const { return root != nullptr; }

    Vector2 get_position() const { return root ? root->get_global_position() : Vector2(); }
    void set_position(const Vector2& p) { if (root) root->set_global_position(p); }

    float get_rotation() const { return root ? root->get_rotation() : 0.0f; }
    void set_rotation(float r) { if (root) root->set_rotation(r); }
};

} // namespace Gd

} // namespace Polaris

#endif // POLARIS_COMPONENTS_GD_H
