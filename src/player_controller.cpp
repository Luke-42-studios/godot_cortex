#include "player_controller.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/node3d.hpp>

using namespace godot;

void PlayerController::_bind_methods() {
    // Properties
    ClassDB::bind_method(D_METHOD("set_move_speed", "speed"), &PlayerController::set_move_speed);
    ClassDB::bind_method(D_METHOD("get_move_speed"), &PlayerController::get_move_speed);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "move_speed", PROPERTY_HINT_RANGE, "0,1000,1"), "set_move_speed", "get_move_speed");

    ClassDB::bind_method(D_METHOD("set_rotation_speed", "speed"), &PlayerController::set_rotation_speed);
    ClassDB::bind_method(D_METHOD("get_rotation_speed"), &PlayerController::get_rotation_speed);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "rotation_speed", PROPERTY_HINT_RANGE, "0,10,0.1"), "set_rotation_speed", "get_rotation_speed");

    // Methods
    ClassDB::bind_method(D_METHOD("get_velocity"), &PlayerController::get_velocity);

    // Signals
    ADD_SIGNAL(MethodInfo("moved", PropertyInfo(Variant::VECTOR2, "velocity")));
}

PlayerController::PlayerController() {
}

PlayerController::~PlayerController() {
}

void PlayerController::on_start() {
    UtilityFunctions::print("PlayerController started on: ", get_parent()->get_name());
}

void PlayerController::on_update(double delta) {
    Input* input = Input::get_singleton();

    // Get input direction
    Vector2 input_dir;
    input_dir.x = input->get_axis("ui_left", "ui_right");
    input_dir.y = input->get_axis("ui_up", "ui_down");

    // Calculate velocity
    velocity = input_dir.normalized() * move_speed;

    // Move the parent node (works for Node2D)
    Node* parent = get_parent();
    if (parent) {
        Node2D* parent2d = Object::cast_to<Node2D>(parent);
        if (parent2d) {
            Vector2 pos = parent2d->get_position();
            pos += velocity * delta;
            parent2d->set_position(pos);

            if (velocity.length() > 0) {
                emit_signal("moved", velocity);
            }
        }

        // Also works for Node3D (XZ plane movement)
        Node3D* parent3d = Object::cast_to<Node3D>(parent);
        if (parent3d) {
            Vector3 pos = parent3d->get_position();
            pos.x += velocity.x * delta;
            pos.z += velocity.y * delta;
            parent3d->set_position(pos);

            if (velocity.length() > 0) {
                emit_signal("moved", velocity);
            }
        }
    }
}

void PlayerController::on_fixed_update(double delta) {
    // Physics-related updates go here
}

void PlayerController::set_move_speed(float speed) {
    move_speed = speed;
}

float PlayerController::get_move_speed() const {
    return move_speed;
}

void PlayerController::set_rotation_speed(float speed) {
    rotation_speed = speed;
}

float PlayerController::get_rotation_speed() const {
    return rotation_speed;
}

Vector2 PlayerController::get_velocity() const {
    return velocity;
}
