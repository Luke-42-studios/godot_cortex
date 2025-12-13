#include "c_node.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void C_Node::_bind_methods() {
    // Bind message property
    ClassDB::bind_method(D_METHOD("set_message", "message"), &C_Node::set_message);
    ClassDB::bind_method(D_METHOD("get_message"), &C_Node::get_message);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "message"), "set_message", "get_message");

    // Bind speed property
    ClassDB::bind_method(D_METHOD("set_speed", "speed"), &C_Node::set_speed);
    ClassDB::bind_method(D_METHOD("get_speed"), &C_Node::get_speed);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed", PROPERTY_HINT_RANGE, "0.1,10.0,0.1"), "set_speed", "get_speed");

    // Bind active property
    ClassDB::bind_method(D_METHOD("set_active", "active"), &C_Node::set_active);
    ClassDB::bind_method(D_METHOD("is_active"), &C_Node::is_active);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "active"), "set_active", "is_active");

    // Bind counter (read-only)
    ClassDB::bind_method(D_METHOD("get_counter"), &C_Node::get_counter);
    ClassDB::bind_method(D_METHOD("reset_counter"), &C_Node::reset_counter);

    // Bind utility method
    ClassDB::bind_method(D_METHOD("get_info"), &C_Node::get_info);

    // Signal when counter changes significantly
    ADD_SIGNAL(MethodInfo("counter_milestone", PropertyInfo(Variant::INT, "value")));
}

C_Node::C_Node() {
    // Initialize the C data structure
    c_node_init(&c_data);
}

C_Node::~C_Node() {
    // Cleanup the C data structure
    c_node_cleanup(&c_data);
}

void C_Node::_ready() {
    UtilityFunctions::print("C_Node ready: ", get_name(), " - ", get_message());
}

void C_Node::_process(double delta) {
    int old_counter = c_node_get_counter(&c_data);

    // Call pure C update function
    int new_counter = c_node_update(&c_data, (float)delta);

    // Emit signal every 100 increments
    if ((new_counter / 100) > (old_counter / 100)) {
        emit_signal("counter_milestone", new_counter);
    }
}

void C_Node::set_message(const String &message) {
    // Convert Godot String to C string and call C function
    CharString utf8 = message.utf8();
    c_node_set_message(&c_data, utf8.get_data());
}

String C_Node::get_message() const {
    // Convert C string to Godot String
    return String(c_node_get_message(&c_data));
}

void C_Node::set_speed(float speed) {
    c_node_set_speed(&c_data, speed);
}

float C_Node::get_speed() const {
    return c_node_get_speed(&c_data);
}

void C_Node::set_active(bool active) {
    c_node_set_active(&c_data, active);
    set_process(active);
}

bool C_Node::is_active() const {
    return c_node_is_active(&c_data);
}

int C_Node::get_counter() const {
    return c_node_get_counter(&c_data);
}

void C_Node::reset_counter() {
    c_node_reset_counter(&c_data);
}

String C_Node::get_info() const {
    return String("C_Node[message='") + get_message() +
           String("', counter=") + String::num_int64(get_counter()) +
           String(", speed=") + String::num(get_speed(), 1) +
           String(", active=") + (is_active() ? "true" : "false") + "]";
}
