#include "cpp_node.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void CPP_Node::_bind_methods() {
    // Bind custom_message property
    ClassDB::bind_method(D_METHOD("set_custom_message", "message"), &CPP_Node::set_custom_message);
    ClassDB::bind_method(D_METHOD("get_custom_message"), &CPP_Node::get_custom_message);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "custom_message"), "set_custom_message", "get_custom_message");

    // Bind processing_enabled property
    ClassDB::bind_method(D_METHOD("set_processing_enabled", "enabled"), &CPP_Node::set_processing_enabled);
    ClassDB::bind_method(D_METHOD("get_processing_enabled"), &CPP_Node::get_processing_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "processing_enabled"), "set_processing_enabled", "get_processing_enabled");

    // Bind update_count (read-only)
    ClassDB::bind_method(D_METHOD("get_update_count"), &CPP_Node::get_update_count);
    ClassDB::bind_method(D_METHOD("reset_update_count"), &CPP_Node::reset_update_count);

    // Bind utility method
    ClassDB::bind_method(D_METHOD("get_info"), &CPP_Node::get_info);

    // Add a signal
    ADD_SIGNAL(MethodInfo("message_changed", PropertyInfo(Variant::STRING, "new_message")));
}

CPP_Node::CPP_Node() {
    custom_message = "test-4";
}

CPP_Node::~CPP_Node() {
}

void CPP_Node::_ready() {
    UtilityFunctions::print("CPP_Node ready 2: ", get_name());
}

void CPP_Node::_process(double delta) {
    if (processing_enabled) {
        update_count++;
    }
}

void CPP_Node::set_custom_message(const String &message) {
    if (custom_message != message) {
        custom_message = message;
        emit_signal("message_changed", message);
    }
}

String CPP_Node::get_custom_message() const {
    return custom_message;
}

void CPP_Node::set_processing_enabled(bool enabled) {
    processing_enabled = enabled;
    set_process(enabled);
}

bool CPP_Node::get_processing_enabled() const {
    return processing_enabled;
}

int CPP_Node::get_update_count() const {
    return update_count;
}

void CPP_Node::reset_update_count() {
    update_count = 0;
}

String CPP_Node::get_info() const {
    return String("CPP_Node: message='") + custom_message +
           String("', updates=") + String::num_int64(update_count) +
           String(", processing=") + (processing_enabled ? "true" : "false");
}
