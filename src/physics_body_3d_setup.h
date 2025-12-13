#ifndef PHYSICS_BODY_3D_SETUP_H
#define PHYSICS_BODY_3D_SETUP_H

#include <godot_cpp/classes/rigid_body3d.hpp>
#include <godot_cpp/classes/static_body3d.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/box_shape3d.hpp>
#include <godot_cpp/classes/sphere_shape3d.hpp>
#include <godot_cpp/classes/capsule_shape3d.hpp>
#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>
#include <godot_cpp/classes/capsule_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>

namespace godot {

/**
 * PhysicsBody3DSetup - Helper to create 3D physics objects in C++
 *
 * In Godot, a physics object is composed of nodes:
 *
 *   RigidBody3D (or StaticBody3D, CharacterBody3D)
 *     ├── MeshInstance3D     (visual)
 *     └── CollisionShape3D   (physics collider)
 *
 * This class creates that structure for you.
 */
class PhysicsBody3DSetup : public RigidBody3D {
    GDCLASS(PhysicsBody3DSetup, RigidBody3D);

public:
    enum ShapeType {
        SHAPE_BOX,
        SHAPE_SPHERE,
        SHAPE_CAPSULE
    };

private:
    MeshInstance3D* mesh_instance = nullptr;
    CollisionShape3D* collision_shape = nullptr;
    ShapeType current_shape = SHAPE_BOX;
    Vector3 shape_size = Vector3(1, 1, 1);
    Color mesh_color = Color(0.8f, 0.2f, 0.2f);

protected:
    static void _bind_methods();

public:
    PhysicsBody3DSetup();
    ~PhysicsBody3DSetup();

    void _ready() override;

    // Setup methods
    void setup_box(const Vector3 &size, const Color &color = Color(0.8f, 0.2f, 0.2f));
    void setup_sphere(float radius, const Color &color = Color(0.2f, 0.8f, 0.2f));
    void setup_capsule(float radius, float height, const Color &color = Color(0.2f, 0.2f, 0.8f));

    // Properties
    void set_shape_type(ShapeType type);
    ShapeType get_shape_type() const;

    void set_shape_size(const Vector3 &size);
    Vector3 get_shape_size() const;

    void set_mesh_color(const Color &color);
    Color get_mesh_color() const;

    // Access child nodes
    MeshInstance3D* get_mesh_instance() const;
    CollisionShape3D* get_collision_shape() const;

private:
    void rebuild_shape();
};

} // namespace godot

VARIANT_ENUM_CAST(godot::PhysicsBody3DSetup::ShapeType);

#endif // PHYSICS_BODY_3D_SETUP_H
