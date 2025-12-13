#include "physics_body_3d_setup.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void PhysicsBody3DSetup::_bind_methods() {
    // Shape type enum
    BIND_ENUM_CONSTANT(SHAPE_BOX);
    BIND_ENUM_CONSTANT(SHAPE_SPHERE);
    BIND_ENUM_CONSTANT(SHAPE_CAPSULE);

    // Setup methods
    ClassDB::bind_method(D_METHOD("setup_box", "size", "color"), &PhysicsBody3DSetup::setup_box, DEFVAL(Color(0.8f, 0.2f, 0.2f)));
    ClassDB::bind_method(D_METHOD("setup_sphere", "radius", "color"), &PhysicsBody3DSetup::setup_sphere, DEFVAL(Color(0.2f, 0.8f, 0.2f)));
    ClassDB::bind_method(D_METHOD("setup_capsule", "radius", "height", "color"), &PhysicsBody3DSetup::setup_capsule, DEFVAL(Color(0.2f, 0.2f, 0.8f)));

    // Properties
    ClassDB::bind_method(D_METHOD("set_shape_type", "type"), &PhysicsBody3DSetup::set_shape_type);
    ClassDB::bind_method(D_METHOD("get_shape_type"), &PhysicsBody3DSetup::get_shape_type);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "shape_type", PROPERTY_HINT_ENUM, "Box,Sphere,Capsule"), "set_shape_type", "get_shape_type");

    ClassDB::bind_method(D_METHOD("set_shape_size", "size"), &PhysicsBody3DSetup::set_shape_size);
    ClassDB::bind_method(D_METHOD("get_shape_size"), &PhysicsBody3DSetup::get_shape_size);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "shape_size"), "set_shape_size", "get_shape_size");

    ClassDB::bind_method(D_METHOD("set_mesh_color", "color"), &PhysicsBody3DSetup::set_mesh_color);
    ClassDB::bind_method(D_METHOD("get_mesh_color"), &PhysicsBody3DSetup::get_mesh_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "mesh_color"), "set_mesh_color", "get_mesh_color");

    // Accessors
    ClassDB::bind_method(D_METHOD("get_mesh_instance"), &PhysicsBody3DSetup::get_mesh_instance);
    ClassDB::bind_method(D_METHOD("get_collision_shape"), &PhysicsBody3DSetup::get_collision_shape);
}

PhysicsBody3DSetup::PhysicsBody3DSetup() {
}

PhysicsBody3DSetup::~PhysicsBody3DSetup() {
}

void PhysicsBody3DSetup::_ready() {
    // Create child nodes if they don't exist
    if (!mesh_instance) {
        mesh_instance = memnew(MeshInstance3D);
        mesh_instance->set_name("MeshInstance3D");
        add_child(mesh_instance);
    }

    if (!collision_shape) {
        collision_shape = memnew(CollisionShape3D);
        collision_shape->set_name("CollisionShape3D");
        add_child(collision_shape);
    }

    rebuild_shape();
}

void PhysicsBody3DSetup::setup_box(const Vector3 &size, const Color &color) {
    current_shape = SHAPE_BOX;
    shape_size = size;
    mesh_color = color;
    rebuild_shape();
}

void PhysicsBody3DSetup::setup_sphere(float radius, const Color &color) {
    current_shape = SHAPE_SPHERE;
    shape_size = Vector3(radius, radius, radius);
    mesh_color = color;
    rebuild_shape();
}

void PhysicsBody3DSetup::setup_capsule(float radius, float height, const Color &color) {
    current_shape = SHAPE_CAPSULE;
    shape_size = Vector3(radius, height, radius);
    mesh_color = color;
    rebuild_shape();
}

void PhysicsBody3DSetup::set_shape_type(ShapeType type) {
    current_shape = type;
    rebuild_shape();
}

PhysicsBody3DSetup::ShapeType PhysicsBody3DSetup::get_shape_type() const {
    return current_shape;
}

void PhysicsBody3DSetup::set_shape_size(const Vector3 &size) {
    shape_size = size;
    rebuild_shape();
}

Vector3 PhysicsBody3DSetup::get_shape_size() const {
    return shape_size;
}

void PhysicsBody3DSetup::set_mesh_color(const Color &color) {
    mesh_color = color;
    rebuild_shape();
}

Color PhysicsBody3DSetup::get_mesh_color() const {
    return mesh_color;
}

MeshInstance3D* PhysicsBody3DSetup::get_mesh_instance() const {
    return mesh_instance;
}

CollisionShape3D* PhysicsBody3DSetup::get_collision_shape() const {
    return collision_shape;
}

void PhysicsBody3DSetup::rebuild_shape() {
    if (!mesh_instance || !collision_shape) {
        return;
    }

    // Create material
    Ref<StandardMaterial3D> material;
    material.instantiate();
    material->set_albedo(mesh_color);

    switch (current_shape) {
        case SHAPE_BOX: {
            // Mesh
            Ref<BoxMesh> box_mesh;
            box_mesh.instantiate();
            box_mesh->set_size(shape_size);
            box_mesh->set_material(material);
            mesh_instance->set_mesh(box_mesh);

            // Collider
            Ref<BoxShape3D> box_shape;
            box_shape.instantiate();
            box_shape->set_size(shape_size);
            collision_shape->set_shape(box_shape);
        } break;

        case SHAPE_SPHERE: {
            float radius = shape_size.x;

            // Mesh
            Ref<SphereMesh> sphere_mesh;
            sphere_mesh.instantiate();
            sphere_mesh->set_radius(radius);
            sphere_mesh->set_height(radius * 2);
            sphere_mesh->set_material(material);
            mesh_instance->set_mesh(sphere_mesh);

            // Collider
            Ref<SphereShape3D> sphere_shape;
            sphere_shape.instantiate();
            sphere_shape->set_radius(radius);
            collision_shape->set_shape(sphere_shape);
        } break;

        case SHAPE_CAPSULE: {
            float radius = shape_size.x;
            float height = shape_size.y;

            // Mesh
            Ref<CapsuleMesh> capsule_mesh;
            capsule_mesh.instantiate();
            capsule_mesh->set_radius(radius);
            capsule_mesh->set_height(height);
            capsule_mesh->set_material(material);
            mesh_instance->set_mesh(capsule_mesh);

            // Collider
            Ref<CapsuleShape3D> capsule_shape;
            capsule_shape.instantiate();
            capsule_shape->set_radius(radius);
            capsule_shape->set_height(height);
            collision_shape->set_shape(capsule_shape);
        } break;
    }
}
