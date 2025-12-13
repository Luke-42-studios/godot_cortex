#ifndef CUSTOM_MESH_3D_H
#define CUSTOM_MESH_3D_H

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/immediate_mesh.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/convex_polygon_shape3d.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>

namespace godot {

/**
 * CustomMesh3D - Procedural mesh with direct rendering access
 *
 * Build custom geometry in C++ with:
 * - Procedural mesh generation
 * - Direct access to ArrayMesh for full control
 * - Optional physics collider (auto-generated from mesh)
 * - Custom shader support
 */
class CustomMesh3D : public Node3D {
    GDCLASS(CustomMesh3D, Node3D);

private:
    // Child nodes
    MeshInstance3D* mesh_instance = nullptr;
    RigidBody3D* physics_body = nullptr;
    CollisionShape3D* collision_shape = nullptr;

    // Resources
    Ref<ArrayMesh> array_mesh;
    Ref<StandardMaterial3D> material;
    Ref<ConvexPolygonShape3D> convex_shape;

    // Mesh building data
    PackedVector3Array vertices;
    PackedVector3Array normals;
    PackedVector2Array uvs;
    PackedColorArray colors;
    PackedInt32Array indices;

    // Properties
    Color albedo_color = Color(0.8f, 0.2f, 0.2f, 1.0f);
    bool use_physics = false;
    bool is_static = true;
    float mass = 1.0f;
    float roughness = 0.5f;
    float metallic = 0.0f;

protected:
    static void _bind_methods();

public:
    CustomMesh3D();
    ~CustomMesh3D();

    void _ready() override;

    // === Mesh Building API ===
    void begin_mesh();
    void add_vertex(const Vector3 &vertex);
    void add_normal(const Vector3 &normal);
    void add_uv(const Vector2 &uv);
    void add_color(const Color &color);
    void add_index(int index);
    void add_triangle(const Vector3 &v1, const Vector3 &v2, const Vector3 &v3);
    void add_quad(const Vector3 &v1, const Vector3 &v2, const Vector3 &v3, const Vector3 &v4);
    void end_mesh();
    void clear_mesh();

    // === Primitive Helpers ===
    void create_box(const Vector3 &size);
    void create_sphere(float radius, int rings = 16, int segments = 32);
    void create_plane(const Vector2 &size, int subdivisions = 1);
    void create_cylinder(float radius, float height, int segments = 32);
    void create_from_arrays(const PackedVector3Array &verts, const PackedInt32Array &inds,
                            const PackedVector3Array &norms = PackedVector3Array(),
                            const PackedVector2Array &tex_uvs = PackedVector2Array());

    // === Material Properties ===
    void set_albedo_color(const Color &color);
    Color get_albedo_color() const;

    void set_roughness(float value);
    float get_roughness() const;

    void set_metallic(float value);
    float get_metallic() const;

    void set_emission(const Color &color, float energy = 1.0f);
    void set_transparency(float alpha);

    // === Physics ===
    void set_use_physics(bool enable);
    bool get_use_physics() const;

    void set_static(bool p_static);
    bool get_static() const;

    void set_mass(float p_mass);
    float get_mass() const;

    void apply_impulse(const Vector3 &impulse, const Vector3 &position = Vector3());
    void apply_force(const Vector3 &force, const Vector3 &position = Vector3());
    void set_linear_velocity(const Vector3 &velocity);
    Vector3 get_linear_velocity() const;

    // === Direct Access ===
    MeshInstance3D* get_mesh_instance() const;
    Ref<ArrayMesh> get_array_mesh() const;
    Ref<StandardMaterial3D> get_material() const;
    RigidBody3D* get_physics_body() const;

    // Low-level server access
    RenderingServer* get_rendering_server() const;
    PhysicsServer3D* get_physics_server() const;
    RID get_mesh_rid() const;
    RID get_instance_rid() const;

    // === Utility ===
    PackedVector3Array get_vertices() const;
    int get_vertex_count() const;
    int get_triangle_count() const;
    AABB get_aabb() const;

private:
    void setup_nodes();
    void update_mesh();
    void update_physics_shape();
    void update_material();
    Vector3 calculate_normal(const Vector3 &v1, const Vector3 &v2, const Vector3 &v3);
};

} // namespace godot

#endif // CUSTOM_MESH_3D_H
