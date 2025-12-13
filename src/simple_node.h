#ifndef SIMPLE_NODE_H
#define SIMPLE_NODE_H

#include "gd_macros.h"
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

namespace godot {

/**
 * SimpleNode - Example using simplified binding macros
 *
 * Compare this to CPP_Node to see how much cleaner the code is.
 * All the repetitive ClassDB::bind_method calls are replaced with
 * simple macros.
 */
class SimpleNode : public Node {
    GDCLASS(SimpleNode, Node);

    // Declare properties with auto-generated getters/setters
    GD_PROPERTY(String, message, "Hello!")
    GD_PROPERTY(int, health, 100)
    GD_PROPERTY(float, speed, 5.0f)
    GD_PROPERTY(bool, active, false)
    GD_PROPERTY(Vector2, velocity, Vector2())
    // Color property with custom setter to update mesh material
private:
    Color _color = Color(1, 1, 1, 1);
public:
    void set_color(Color value) {
        _color = value;
        // Update material color if mesh exists
        if (mesh_instance) {
            Ref<StandardMaterial3D> mat = mesh_instance->get_surface_override_material(0);
            if (mat.is_valid()) {
                mat->set_albedo(_color);
            }
        }
    }
    Color get_color() const { return _color; }

private:
    int tick_count = 0;
    MeshInstance3D* mesh_instance = nullptr;

protected:
    static void _bind_methods() {
        // Properties - one line each!
        GD_BIND_PROPERTY(SimpleNode, String, message);
        GD_BIND_PROPERTY(SimpleNode, int, health);
        GD_BIND_PROPERTY_HINT(SimpleNode, float, speed, PROPERTY_HINT_RANGE, "0,100,0.1");
        GD_BIND_PROPERTY(SimpleNode, bool, active);
        GD_BIND_PROPERTY(SimpleNode, Vector2, velocity);
        GD_BIND_PROPERTY(SimpleNode, Color, color);

        // Methods
        GD_BIND_METHOD(SimpleNode, get_tick_count);
        GD_BIND_METHOD(SimpleNode, reset);
        GD_BIND_METHOD_1(SimpleNode, take_damage, amount);
        GD_BIND_METHOD(SimpleNode, get_info);

        // Signals
        GD_BIND_SIGNAL(tick);
        GD_BIND_SIGNAL_1(damaged, Variant::INT, amount);
        GD_BIND_SIGNAL(died);
    }

public:
    SimpleNode() {}
    ~SimpleNode() {}

    void _ready() override {
        UtilityFunctions::print("SimpleNode ready: ", get_name());

        // Create a MeshInstance3D as a child node
        mesh_instance = memnew(MeshInstance3D);
        mesh_instance->set_name("BoxMesh");

        // Create a BoxMesh resource
        Ref<BoxMesh> box_mesh;
        box_mesh.instantiate();
        box_mesh->set_size(Vector3(1.0f, 1.0f, 1.0f));

        // Create a material and set its color from our property
        Ref<StandardMaterial3D> material;
        material.instantiate();
        material->set_albedo(_color);

        // Assign mesh and material
        mesh_instance->set_mesh(box_mesh);
        mesh_instance->set_surface_override_material(0, material);

        // Add as child - this makes it part of the scene
        add_child(mesh_instance);

        // Set owner so it appears in the editor scene tree (not just Remote)
        if (get_tree() && get_tree()->get_edited_scene_root()) {
            mesh_instance->set_owner(get_tree()->get_edited_scene_root());
        }
    }

    void _process(double delta) override {
        if (_active) {
            tick_count++;
            _velocity += Vector2(0, -9.8f * delta); // Simple gravity
            emit_signal("tick");
        }
    }

    // Custom methods
    int get_tick_count() const { return tick_count; }

    void reset() {
        tick_count = 0;
        _health = 100;
        _velocity = Vector2();
    }

    void take_damage(int amount) {
        _health -= amount;
        emit_signal("damaged", amount);
        if (_health <= 0) {
            _health = 0;
            emit_signal("died");
        }
    }

    String get_info() const {
        return String("SimpleNode[health=") + String::num_int64(_health) +
               ", speed=" + String::num(_speed, 1) +
               ", ticks=" + String::num_int64(tick_count) + "]";
    }
};

} // namespace godot

#endif // SIMPLE_NODE_H
