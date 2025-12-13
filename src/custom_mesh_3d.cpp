#include "custom_mesh_3d.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void CustomMesh3D::_bind_methods() {
    // Mesh building
    ClassDB::bind_method(D_METHOD("begin_mesh"), &CustomMesh3D::begin_mesh);
    ClassDB::bind_method(D_METHOD("add_vertex", "vertex"), &CustomMesh3D::add_vertex);
    ClassDB::bind_method(D_METHOD("add_normal", "normal"), &CustomMesh3D::add_normal);
    ClassDB::bind_method(D_METHOD("add_uv", "uv"), &CustomMesh3D::add_uv);
    ClassDB::bind_method(D_METHOD("add_color", "color"), &CustomMesh3D::add_color);
    ClassDB::bind_method(D_METHOD("add_index", "index"), &CustomMesh3D::add_index);
    ClassDB::bind_method(D_METHOD("add_triangle", "v1", "v2", "v3"), &CustomMesh3D::add_triangle);
    ClassDB::bind_method(D_METHOD("add_quad", "v1", "v2", "v3", "v4"), &CustomMesh3D::add_quad);
    ClassDB::bind_method(D_METHOD("end_mesh"), &CustomMesh3D::end_mesh);
    ClassDB::bind_method(D_METHOD("clear_mesh"), &CustomMesh3D::clear_mesh);

    // Primitives
    ClassDB::bind_method(D_METHOD("create_box", "size"), &CustomMesh3D::create_box);
    ClassDB::bind_method(D_METHOD("create_sphere", "radius", "rings", "segments"), &CustomMesh3D::create_sphere, DEFVAL(16), DEFVAL(32));
    ClassDB::bind_method(D_METHOD("create_plane", "size", "subdivisions"), &CustomMesh3D::create_plane, DEFVAL(1));
    ClassDB::bind_method(D_METHOD("create_cylinder", "radius", "height", "segments"), &CustomMesh3D::create_cylinder, DEFVAL(32));
    ClassDB::bind_method(D_METHOD("create_from_arrays", "vertices", "indices", "normals", "uvs"), &CustomMesh3D::create_from_arrays, DEFVAL(PackedVector3Array()), DEFVAL(PackedVector2Array()));

    // Material
    ClassDB::bind_method(D_METHOD("set_albedo_color", "color"), &CustomMesh3D::set_albedo_color);
    ClassDB::bind_method(D_METHOD("get_albedo_color"), &CustomMesh3D::get_albedo_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "albedo_color"), "set_albedo_color", "get_albedo_color");

    ClassDB::bind_method(D_METHOD("set_roughness", "value"), &CustomMesh3D::set_roughness);
    ClassDB::bind_method(D_METHOD("get_roughness"), &CustomMesh3D::get_roughness);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "roughness", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_roughness", "get_roughness");

    ClassDB::bind_method(D_METHOD("set_metallic", "value"), &CustomMesh3D::set_metallic);
    ClassDB::bind_method(D_METHOD("get_metallic"), &CustomMesh3D::get_metallic);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "metallic", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_metallic", "get_metallic");

    ClassDB::bind_method(D_METHOD("set_emission", "color", "energy"), &CustomMesh3D::set_emission, DEFVAL(1.0f));
    ClassDB::bind_method(D_METHOD("set_transparency", "alpha"), &CustomMesh3D::set_transparency);

    // Physics
    ClassDB::bind_method(D_METHOD("set_use_physics", "enable"), &CustomMesh3D::set_use_physics);
    ClassDB::bind_method(D_METHOD("get_use_physics"), &CustomMesh3D::get_use_physics);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_physics"), "set_use_physics", "get_use_physics");

    ClassDB::bind_method(D_METHOD("set_static", "is_static"), &CustomMesh3D::set_static);
    ClassDB::bind_method(D_METHOD("get_static"), &CustomMesh3D::get_static);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_static"), "set_static", "get_static");

    ClassDB::bind_method(D_METHOD("set_mass", "mass"), &CustomMesh3D::set_mass);
    ClassDB::bind_method(D_METHOD("get_mass"), &CustomMesh3D::get_mass);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mass", PROPERTY_HINT_RANGE, "0.01,1000,0.01"), "set_mass", "get_mass");

    ClassDB::bind_method(D_METHOD("apply_impulse", "impulse", "position"), &CustomMesh3D::apply_impulse, DEFVAL(Vector3()));
    ClassDB::bind_method(D_METHOD("apply_force", "force", "position"), &CustomMesh3D::apply_force, DEFVAL(Vector3()));
    ClassDB::bind_method(D_METHOD("set_linear_velocity", "velocity"), &CustomMesh3D::set_linear_velocity);
    ClassDB::bind_method(D_METHOD("get_linear_velocity"), &CustomMesh3D::get_linear_velocity);

    // Direct access
    ClassDB::bind_method(D_METHOD("get_mesh_instance"), &CustomMesh3D::get_mesh_instance);
    ClassDB::bind_method(D_METHOD("get_array_mesh"), &CustomMesh3D::get_array_mesh);
    ClassDB::bind_method(D_METHOD("get_material"), &CustomMesh3D::get_material);
    ClassDB::bind_method(D_METHOD("get_physics_body"), &CustomMesh3D::get_physics_body);
    ClassDB::bind_method(D_METHOD("get_mesh_rid"), &CustomMesh3D::get_mesh_rid);
    ClassDB::bind_method(D_METHOD("get_instance_rid"), &CustomMesh3D::get_instance_rid);

    // Utility
    ClassDB::bind_method(D_METHOD("get_vertices"), &CustomMesh3D::get_vertices);
    ClassDB::bind_method(D_METHOD("get_vertex_count"), &CustomMesh3D::get_vertex_count);
    ClassDB::bind_method(D_METHOD("get_triangle_count"), &CustomMesh3D::get_triangle_count);
    ClassDB::bind_method(D_METHOD("get_aabb"), &CustomMesh3D::get_aabb);
}

CustomMesh3D::CustomMesh3D() {
}

CustomMesh3D::~CustomMesh3D() {
}

void CustomMesh3D::_ready() {
    setup_nodes();
    // Create a default box so something is visible
    create_box(Vector3(1, 1, 1));
}

void CustomMesh3D::setup_nodes() {
    // Create MeshInstance3D
    mesh_instance = memnew(MeshInstance3D);
    mesh_instance->set_name("MeshInstance");
    add_child(mesh_instance);

    // Create ArrayMesh
    array_mesh.instantiate();
    mesh_instance->set_mesh(array_mesh);

    // Create material
    material.instantiate();
    material->set_albedo(albedo_color);
    material->set_roughness(roughness);
    material->set_metallic(metallic);

    UtilityFunctions::print("CustomMesh3D: Setup complete");
}

void CustomMesh3D::begin_mesh() {
    clear_mesh();
}

void CustomMesh3D::add_vertex(const Vector3 &vertex) {
    vertices.push_back(vertex);
}

void CustomMesh3D::add_normal(const Vector3 &normal) {
    normals.push_back(normal);
}

void CustomMesh3D::add_uv(const Vector2 &uv) {
    uvs.push_back(uv);
}

void CustomMesh3D::add_color(const Color &color) {
    colors.push_back(color);
}

void CustomMesh3D::add_index(int index) {
    indices.push_back(index);
}

Vector3 CustomMesh3D::calculate_normal(const Vector3 &v1, const Vector3 &v2, const Vector3 &v3) {
    return (v2 - v1).cross(v3 - v1).normalized();
}

void CustomMesh3D::add_triangle(const Vector3 &v1, const Vector3 &v2, const Vector3 &v3) {
    int base = vertices.size();

    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v3);

    Vector3 normal = calculate_normal(v1, v2, v3);
    normals.push_back(normal);
    normals.push_back(normal);
    normals.push_back(normal);

    indices.push_back(base);
    indices.push_back(base + 1);
    indices.push_back(base + 2);
}

void CustomMesh3D::add_quad(const Vector3 &v1, const Vector3 &v2, const Vector3 &v3, const Vector3 &v4) {
    // Two triangles: v1-v2-v3 and v1-v3-v4
    add_triangle(v1, v2, v3);
    add_triangle(v1, v3, v4);
}

void CustomMesh3D::end_mesh() {
    update_mesh();
    if (use_physics) {
        update_physics_shape();
    }
}

void CustomMesh3D::clear_mesh() {
    vertices.clear();
    normals.clear();
    uvs.clear();
    colors.clear();
    indices.clear();
}

void CustomMesh3D::update_mesh() {
    if (vertices.size() == 0) {
        return;
    }

    // Clear existing surfaces
    array_mesh->clear_surfaces();

    // Build surface arrays
    Array arrays;
    arrays.resize(Mesh::ARRAY_MAX);
    arrays[Mesh::ARRAY_VERTEX] = vertices;

    if (normals.size() == vertices.size()) {
        arrays[Mesh::ARRAY_NORMAL] = normals;
    }
    if (uvs.size() == vertices.size()) {
        arrays[Mesh::ARRAY_TEX_UV] = uvs;
    }
    if (colors.size() == vertices.size()) {
        arrays[Mesh::ARRAY_COLOR] = colors;
    }
    if (indices.size() > 0) {
        arrays[Mesh::ARRAY_INDEX] = indices;
    }

    // Add surface
    array_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

    // Apply material
    if (material.is_valid()) {
        array_mesh->surface_set_material(0, material);
    }

    UtilityFunctions::print("CustomMesh3D: Mesh updated with ", vertices.size(), " vertices, ", indices.size() / 3, " triangles");
}

void CustomMesh3D::update_physics_shape() {
    if (!use_physics || vertices.size() == 0) {
        return;
    }

    // Create physics body if needed
    if (!physics_body) {
        physics_body = memnew(RigidBody3D);
        physics_body->set_name("PhysicsBody");
        add_child(physics_body);

        collision_shape = memnew(CollisionShape3D);
        collision_shape->set_name("CollisionShape");
        physics_body->add_child(collision_shape);

        // Move mesh under physics body
        remove_child(mesh_instance);
        physics_body->add_child(mesh_instance);
    }

    // Set body mode
    if (is_static) {
        physics_body->set_freeze_enabled(true);
        physics_body->set_freeze_mode(RigidBody3D::FREEZE_MODE_STATIC);
    } else {
        physics_body->set_freeze_enabled(false);
        physics_body->set_mass(mass);
    }

    // Create convex shape from vertices
    convex_shape.instantiate();
    convex_shape->set_points(vertices);
    collision_shape->set_shape(convex_shape);
}

void CustomMesh3D::update_material() {
    if (material.is_valid()) {
        material->set_albedo(albedo_color);
        material->set_roughness(roughness);
        material->set_metallic(metallic);
    }
}

// === Primitive Helpers ===

void CustomMesh3D::create_box(const Vector3 &size) {
    begin_mesh();

    Vector3 h = size * 0.5f;

    // Front face (+Z)
    add_quad(Vector3(-h.x, -h.y, h.z), Vector3(h.x, -h.y, h.z), Vector3(h.x, h.y, h.z), Vector3(-h.x, h.y, h.z));
    // Back face (-Z)
    add_quad(Vector3(h.x, -h.y, -h.z), Vector3(-h.x, -h.y, -h.z), Vector3(-h.x, h.y, -h.z), Vector3(h.x, h.y, -h.z));
    // Top face (+Y)
    add_quad(Vector3(-h.x, h.y, h.z), Vector3(h.x, h.y, h.z), Vector3(h.x, h.y, -h.z), Vector3(-h.x, h.y, -h.z));
    // Bottom face (-Y)
    add_quad(Vector3(-h.x, -h.y, -h.z), Vector3(h.x, -h.y, -h.z), Vector3(h.x, -h.y, h.z), Vector3(-h.x, -h.y, h.z));
    // Right face (+X)
    add_quad(Vector3(h.x, -h.y, h.z), Vector3(h.x, -h.y, -h.z), Vector3(h.x, h.y, -h.z), Vector3(h.x, h.y, h.z));
    // Left face (-X)
    add_quad(Vector3(-h.x, -h.y, -h.z), Vector3(-h.x, -h.y, h.z), Vector3(-h.x, h.y, h.z), Vector3(-h.x, h.y, -h.z));

    end_mesh();
}

void CustomMesh3D::create_sphere(float radius, int rings, int segments) {
    begin_mesh();

    for (int i = 0; i <= rings; i++) {
        float v = (float)i / rings;
        float phi = v * Math_PI;

        for (int j = 0; j <= segments; j++) {
            float u = (float)j / segments;
            float theta = u * Math_PI * 2;

            float x = cos(theta) * sin(phi);
            float y = cos(phi);
            float z = sin(theta) * sin(phi);

            vertices.push_back(Vector3(x, y, z) * radius);
            normals.push_back(Vector3(x, y, z));
            uvs.push_back(Vector2(u, v));
        }
    }

    for (int i = 0; i < rings; i++) {
        for (int j = 0; j < segments; j++) {
            int a = i * (segments + 1) + j;
            int b = a + segments + 1;

            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(a + 1);

            indices.push_back(b);
            indices.push_back(b + 1);
            indices.push_back(a + 1);
        }
    }

    end_mesh();
}

void CustomMesh3D::create_plane(const Vector2 &size, int subdivisions) {
    begin_mesh();

    Vector2 h = size * 0.5f;
    float step = 1.0f / subdivisions;

    for (int y = 0; y <= subdivisions; y++) {
        for (int x = 0; x <= subdivisions; x++) {
            float u = x * step;
            float v = y * step;
            Vector3 pos(-h.x + size.x * u, 0, -h.y + size.y * v);
            vertices.push_back(pos);
            normals.push_back(Vector3(0, 1, 0));
            uvs.push_back(Vector2(u, v));
        }
    }

    for (int y = 0; y < subdivisions; y++) {
        for (int x = 0; x < subdivisions; x++) {
            int i = y * (subdivisions + 1) + x;
            indices.push_back(i);
            indices.push_back(i + subdivisions + 1);
            indices.push_back(i + 1);

            indices.push_back(i + subdivisions + 1);
            indices.push_back(i + subdivisions + 2);
            indices.push_back(i + 1);
        }
    }

    end_mesh();
}

void CustomMesh3D::create_cylinder(float radius, float height, int segments) {
    begin_mesh();

    float half_h = height * 0.5f;

    // Top cap center
    int top_center = vertices.size();
    vertices.push_back(Vector3(0, half_h, 0));
    normals.push_back(Vector3(0, 1, 0));

    // Top cap ring
    int top_ring_start = vertices.size();
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / segments * Math_PI * 2;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;
        vertices.push_back(Vector3(x, half_h, z));
        normals.push_back(Vector3(0, 1, 0));
    }

    // Top cap triangles
    for (int i = 0; i < segments; i++) {
        indices.push_back(top_center);
        indices.push_back(top_ring_start + i + 1);
        indices.push_back(top_ring_start + i);
    }

    // Bottom cap center
    int bottom_center = vertices.size();
    vertices.push_back(Vector3(0, -half_h, 0));
    normals.push_back(Vector3(0, -1, 0));

    // Bottom cap ring
    int bottom_ring_start = vertices.size();
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / segments * Math_PI * 2;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;
        vertices.push_back(Vector3(x, -half_h, z));
        normals.push_back(Vector3(0, -1, 0));
    }

    // Bottom cap triangles
    for (int i = 0; i < segments; i++) {
        indices.push_back(bottom_center);
        indices.push_back(bottom_ring_start + i);
        indices.push_back(bottom_ring_start + i + 1);
    }

    // Side
    int side_start = vertices.size();
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / segments * Math_PI * 2;
        float x = cos(angle);
        float z = sin(angle);

        // Top vertex
        vertices.push_back(Vector3(x * radius, half_h, z * radius));
        normals.push_back(Vector3(x, 0, z));

        // Bottom vertex
        vertices.push_back(Vector3(x * radius, -half_h, z * radius));
        normals.push_back(Vector3(x, 0, z));
    }

    // Side triangles
    for (int i = 0; i < segments; i++) {
        int top1 = side_start + i * 2;
        int bot1 = top1 + 1;
        int top2 = top1 + 2;
        int bot2 = top1 + 3;

        indices.push_back(top1);
        indices.push_back(bot1);
        indices.push_back(top2);

        indices.push_back(bot1);
        indices.push_back(bot2);
        indices.push_back(top2);
    }

    end_mesh();
}

void CustomMesh3D::create_from_arrays(const PackedVector3Array &verts, const PackedInt32Array &inds,
                                       const PackedVector3Array &norms, const PackedVector2Array &tex_uvs) {
    begin_mesh();
    vertices = verts;
    indices = inds;
    if (norms.size() > 0) {
        normals = norms;
    }
    if (tex_uvs.size() > 0) {
        uvs = tex_uvs;
    }
    end_mesh();
}

// === Material Properties ===

void CustomMesh3D::set_albedo_color(const Color &color) {
    albedo_color = color;
    update_material();
}

Color CustomMesh3D::get_albedo_color() const {
    return albedo_color;
}

void CustomMesh3D::set_roughness(float value) {
    roughness = value;
    update_material();
}

float CustomMesh3D::get_roughness() const {
    return roughness;
}

void CustomMesh3D::set_metallic(float value) {
    metallic = value;
    update_material();
}

float CustomMesh3D::get_metallic() const {
    return metallic;
}

void CustomMesh3D::set_emission(const Color &color, float energy) {
    if (material.is_valid()) {
        material->set_feature(StandardMaterial3D::FEATURE_EMISSION, true);
        material->set_emission(color);
        material->set_emission_energy_multiplier(energy);
    }
}

void CustomMesh3D::set_transparency(float alpha) {
    if (material.is_valid()) {
        albedo_color.a = alpha;
        material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
        material->set_albedo(albedo_color);
    }
}

// === Physics ===

void CustomMesh3D::set_use_physics(bool enable) {
    use_physics = enable;
    if (enable && vertices.size() > 0) {
        update_physics_shape();
    }
}

bool CustomMesh3D::get_use_physics() const {
    return use_physics;
}

void CustomMesh3D::set_static(bool p_static) {
    is_static = p_static;
    if (physics_body) {
        if (is_static) {
            physics_body->set_freeze_enabled(true);
            physics_body->set_freeze_mode(RigidBody3D::FREEZE_MODE_STATIC);
        } else {
            physics_body->set_freeze_enabled(false);
        }
    }
}

bool CustomMesh3D::get_static() const {
    return is_static;
}

void CustomMesh3D::set_mass(float p_mass) {
    mass = p_mass;
    if (physics_body) {
        physics_body->set_mass(mass);
    }
}

float CustomMesh3D::get_mass() const {
    return mass;
}

void CustomMesh3D::apply_impulse(const Vector3 &impulse, const Vector3 &position) {
    if (physics_body) {
        physics_body->apply_impulse(impulse, position);
    }
}

void CustomMesh3D::apply_force(const Vector3 &force, const Vector3 &position) {
    if (physics_body) {
        physics_body->apply_force(force, position);
    }
}

void CustomMesh3D::set_linear_velocity(const Vector3 &velocity) {
    if (physics_body) {
        physics_body->set_linear_velocity(velocity);
    }
}

Vector3 CustomMesh3D::get_linear_velocity() const {
    if (physics_body) {
        return physics_body->get_linear_velocity();
    }
    return Vector3();
}

// === Direct Access ===

MeshInstance3D* CustomMesh3D::get_mesh_instance() const {
    return mesh_instance;
}

Ref<ArrayMesh> CustomMesh3D::get_array_mesh() const {
    return array_mesh;
}

Ref<StandardMaterial3D> CustomMesh3D::get_material() const {
    return material;
}

RigidBody3D* CustomMesh3D::get_physics_body() const {
    return physics_body;
}

RenderingServer* CustomMesh3D::get_rendering_server() const {
    return RenderingServer::get_singleton();
}

PhysicsServer3D* CustomMesh3D::get_physics_server() const {
    return PhysicsServer3D::get_singleton();
}

RID CustomMesh3D::get_mesh_rid() const {
    if (array_mesh.is_valid()) {
        return array_mesh->get_rid();
    }
    return RID();
}

RID CustomMesh3D::get_instance_rid() const {
    if (mesh_instance) {
        return mesh_instance->get_instance();
    }
    return RID();
}

// === Utility ===

PackedVector3Array CustomMesh3D::get_vertices() const {
    return vertices;
}

int CustomMesh3D::get_vertex_count() const {
    return vertices.size();
}

int CustomMesh3D::get_triangle_count() const {
    return indices.size() / 3;
}

AABB CustomMesh3D::get_aabb() const {
    if (array_mesh.is_valid()) {
        return array_mesh->get_aabb();
    }
    return AABB();
}
