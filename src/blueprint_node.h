#ifndef BLUEPRINT_NODE_H
#define BLUEPRINT_NODE_H

#include "gd_macros.h"
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/resource.hpp>

namespace godot {

// Forward declaration
class BlueprintNode;

// ============================================================================
// BlueprintLogic - Base class for interchangeable behavior
// ============================================================================
// This is a Resource so it can be assigned in the editor!
class BlueprintLogic : public Resource {
    GDCLASS(BlueprintLogic, Resource);

protected:
    BlueprintNode* owner = nullptr;

    static void _bind_methods() {
        // Base class bindings - subclasses override these
        ClassDB::bind_method(D_METHOD("on_ready"), &BlueprintLogic::on_ready);
        ClassDB::bind_method(D_METHOD("on_process", "delta"), &BlueprintLogic::on_process);
        ClassDB::bind_method(D_METHOD("on_interact"), &BlueprintLogic::on_interact);
        ClassDB::bind_method(D_METHOD("on_damage", "amount"), &BlueprintLogic::on_damage);
    }

public:
    BlueprintLogic() {}
    virtual ~BlueprintLogic() {}

    // Called by BlueprintNode to set the owner reference
    void set_owner_node(BlueprintNode* node) { owner = node; }
    BlueprintNode* get_owner_node() const { return owner; }

    // Virtual methods - override these in subclasses
    virtual void on_ready() {}
    virtual void on_process(double delta) {}
    virtual void on_interact() {}
    virtual void on_damage(int amount) {}
};

// ============================================================================
// BlueprintNode - The "template" that defines structure
// ============================================================================
// This is the node you add in the editor. It has:
// - Visual representation (mesh, etc.)
// - Properties exposed to editor
// - A "logic" slot where you assign a BlueprintLogic resource
class BlueprintNode : public Node3D {
    GDCLASS(BlueprintNode, Node3D);

    // Properties exposed to editor
    GD_PROPERTY(String, display_name, "Blueprint")
    GD_PROPERTY(int, health, 100)
    GD_PROPERTY(float, speed, 5.0f)
    GD_PROPERTY(bool, interactable, true)
    GD_PROPERTY(Color, color, Color(1, 1, 1, 1))

private:
    // The logic resource - assignable in editor!
    Ref<BlueprintLogic> logic;

    // Visual components
    MeshInstance3D* mesh_instance = nullptr;

protected:
    static void _bind_methods() {
        // Properties
        GD_BIND_PROPERTY(BlueprintNode, String, display_name);
        GD_BIND_PROPERTY(BlueprintNode, int, health);
        GD_BIND_PROPERTY_HINT(BlueprintNode, float, speed, PROPERTY_HINT_RANGE, "0,100,0.1");
        GD_BIND_PROPERTY(BlueprintNode, bool, interactable);
        GD_BIND_PROPERTY(BlueprintNode, Color, color);

        // The logic resource property - THIS IS THE KEY!
        ClassDB::bind_method(D_METHOD("set_logic", "logic"), &BlueprintNode::set_logic);
        ClassDB::bind_method(D_METHOD("get_logic"), &BlueprintNode::get_logic);
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "logic", PROPERTY_HINT_RESOURCE_TYPE, "BlueprintLogic"),
                     "set_logic", "get_logic");

        // Methods that can be called from logic or externally
        GD_BIND_METHOD(BlueprintNode, interact);
        GD_BIND_METHOD_1(BlueprintNode, take_damage, amount);
        GD_BIND_METHOD(BlueprintNode, get_info);

        // Signals
        GD_BIND_SIGNAL(interacted);
        GD_BIND_SIGNAL_1(damaged, Variant::INT, amount);
        GD_BIND_SIGNAL(died);
    }

public:
    BlueprintNode() {}
    ~BlueprintNode() {}

    // Logic getter/setter
    void set_logic(const Ref<BlueprintLogic>& p_logic) {
        logic = p_logic;
        if (logic.is_valid()) {
            logic->set_owner_node(this);
        }
    }
    Ref<BlueprintLogic> get_logic() const { return logic; }

    void _ready() override {
        UtilityFunctions::print("BlueprintNode ready: ", _display_name);

        // Create visual representation
        setup_visuals();

        // Initialize logic if assigned
        if (logic.is_valid()) {
            logic->set_owner_node(this);
            logic->on_ready();
        }
    }

    void _process(double delta) override {
        // Delegate to logic
        if (logic.is_valid()) {
            logic->on_process(delta);
        }
    }

    // Public methods that logic can use or external code can call
    void interact() {
        if (!_interactable) return;

        emit_signal("interacted");
        if (logic.is_valid()) {
            logic->on_interact();
        }
    }

    void take_damage(int amount) {
        _health -= amount;
        emit_signal("damaged", amount);

        if (logic.is_valid()) {
            logic->on_damage(amount);
        }

        if (_health <= 0) {
            _health = 0;
            emit_signal("died");
        }
    }

    String get_info() const {
        String logic_name = logic.is_valid() ? logic->get_class() : "None";
        return String("Blueprint[") + _display_name +
               ", health=" + String::num_int64(_health) +
               ", logic=" + logic_name + "]";
    }

    // Accessors for logic to use
    MeshInstance3D* get_mesh_instance() const { return mesh_instance; }

    void set_mesh_color(const Color& c) {
        if (mesh_instance) {
            Ref<StandardMaterial3D> mat = mesh_instance->get_surface_override_material(0);
            if (mat.is_valid()) {
                mat->set_albedo(c);
            }
        }
    }

private:
    void setup_visuals() {
        mesh_instance = memnew(MeshInstance3D);
        mesh_instance->set_name("Mesh");

        Ref<BoxMesh> box_mesh;
        box_mesh.instantiate();
        box_mesh->set_size(Vector3(1.0f, 1.0f, 1.0f));

        Ref<StandardMaterial3D> material;
        material.instantiate();
        material->set_albedo(_color);

        mesh_instance->set_mesh(box_mesh);
        mesh_instance->set_surface_override_material(0, material);
        add_child(mesh_instance);

        if (get_tree() && get_tree()->get_edited_scene_root()) {
            mesh_instance->set_owner(get_tree()->get_edited_scene_root());
        }
    }
};

} // namespace godot

#endif // BLUEPRINT_NODE_H
