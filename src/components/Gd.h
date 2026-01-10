#ifndef POLARIS_COMPONENTS_GD_H
#define POLARIS_COMPONENTS_GD_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/shape3d.hpp>
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
    String get_name() const { return root ? String(root->get_name()) : String(); }
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

// -----------------------------------------------------------------------------
// CharacterBody3D - Physics body with move_and_slide
// -----------------------------------------------------------------------------
// Use for pawns that need physics-based movement.

struct CharacterBody3D {
    godot::CharacterBody3D* ptr = nullptr;

    static CharacterBody3D create(godot::Node* n) {
        return { Object::cast_to<godot::CharacterBody3D>(n) };
    }

    bool is_valid() const { return ptr != nullptr; }
    void unbind() { ptr = nullptr; }

    // Physics helpers
    Vector3 get_velocity() const { return ptr ? ptr->get_velocity() : Vector3(); }
    void set_velocity(const Vector3& v) { if (ptr) ptr->set_velocity(v); }

    bool move_and_slide() { return ptr ? ptr->move_and_slide() : false; }
    bool is_on_floor() const { return ptr ? ptr->is_on_floor() : false; }
    bool is_on_wall() const { return ptr ? ptr->is_on_wall() : false; }
    bool is_on_ceiling() const { return ptr ? ptr->is_on_ceiling() : false; }

    Vector3 get_floor_normal() const { return ptr ? ptr->get_floor_normal() : Vector3(0, 1, 0); }
    Vector3 get_wall_normal() const { return ptr ? ptr->get_wall_normal() : Vector3(); }
};

// -----------------------------------------------------------------------------
// Camera3D - Camera reference for player view
// -----------------------------------------------------------------------------
// Use for entities that control camera rotation (pitch).

struct Camera3D {
    godot::Camera3D* ptr = nullptr;

    static Camera3D create(godot::Camera3D* c) {
        return { c };
    }

    static Camera3D create(godot::Node* n) {
        return { Object::cast_to<godot::Camera3D>(n) };
    }

    bool is_valid() const { return ptr != nullptr; }
    void unbind() { ptr = nullptr; }

    // Transform helpers
    Vector3 get_rotation() const { return ptr ? ptr->get_rotation() : Vector3(); }
    void set_rotation(const Vector3& r) { if (ptr) ptr->set_rotation(r); }

    float get_fov() const { return ptr ? ptr->get_fov() : 75.0f; }
    void set_fov(float f) { if (ptr) ptr->set_fov(f); }

    void make_current() { if (ptr) ptr->make_current(); }
    bool is_current() const { return ptr ? ptr->is_current() : false; }
};

// -----------------------------------------------------------------------------
// CollisionShape3D - Collision shape for physics bodies
// -----------------------------------------------------------------------------
// Use for entities that need dynamic collision shape resizing (e.g., crouch).

struct CollisionShape3D {
    godot::CollisionShape3D* ptr = nullptr;

    static CollisionShape3D create(godot::CollisionShape3D* p) { return { p }; }

    static CollisionShape3D create(godot::Node* n) {
        return { Object::cast_to<godot::CollisionShape3D>(n) };
    }

    bool is_valid() const { return ptr != nullptr; }
    void unbind() { ptr = nullptr; }

    // Shape access
    Ref<Shape3D> get_shape() const { return ptr ? ptr->get_shape() : Ref<Shape3D>(); }
    void set_shape(const Ref<Shape3D>& shape) { if (ptr) ptr->set_shape(shape); }

    // Transform helpers
    Vector3 get_position() const { return ptr ? ptr->get_position() : Vector3(); }
    void set_position(const Vector3& p) { if (ptr) ptr->set_position(p); }
};

} // namespace Gd

// =============================================================================
// Tag:: Components - Zero-sized marker tags for filtering
// =============================================================================

namespace Tag {

struct Player {};   // Marks player-controlled entities

} // namespace Tag

} // namespace Polaris

#endif // POLARIS_COMPONENTS_GD_H
