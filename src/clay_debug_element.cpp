#include "clay_debug_element.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void ClayDebugElement::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_element_id", "id"), &ClayDebugElement::set_element_id);
    ClassDB::bind_method(D_METHOD("get_element_id"), &ClayDebugElement::get_element_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "element_id"), "set_element_id", "get_element_id");

    ClassDB::bind_method(D_METHOD("set_element_hash", "hash"), &ClayDebugElement::set_element_hash);
    ClassDB::bind_method(D_METHOD("get_element_hash"), &ClayDebugElement::get_element_hash);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "element_hash"), "set_element_hash", "get_element_hash");

    ClassDB::bind_method(D_METHOD("set_bounding_box", "box"), &ClayDebugElement::set_bounding_box);
    ClassDB::bind_method(D_METHOD("get_bounding_box"), &ClayDebugElement::get_bounding_box);
    ADD_PROPERTY(PropertyInfo(Variant::RECT2, "bounding_box"), "set_bounding_box", "get_bounding_box");

    ClassDB::bind_method(D_METHOD("set_element_type", "type"), &ClayDebugElement::set_element_type);
    ClassDB::bind_method(D_METHOD("get_element_type"), &ClayDebugElement::get_element_type);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "element_type"), "set_element_type", "get_element_type");

    ClassDB::bind_method(D_METHOD("set_z_index", "z"), &ClayDebugElement::set_z_index);
    ClassDB::bind_method(D_METHOD("get_z_index"), &ClayDebugElement::get_z_index);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "z_index"), "set_z_index", "get_z_index");

    ClassDB::bind_method(D_METHOD("get_debug_info"), &ClayDebugElement::get_debug_info);
}

ClayDebugElement::ClayDebugElement() {
}

ClayDebugElement::~ClayDebugElement() {
}

void ClayDebugElement::set_element_id(const String& id) {
    element_id = id;
}

String ClayDebugElement::get_element_id() const {
    return element_id;
}

void ClayDebugElement::set_element_hash(uint32_t hash) {
    element_hash = hash;
}

uint32_t ClayDebugElement::get_element_hash() const {
    return element_hash;
}

void ClayDebugElement::set_bounding_box(const Rect2& box) {
    bounding_box = box;
}

Rect2 ClayDebugElement::get_bounding_box() const {
    return bounding_box;
}

void ClayDebugElement::set_element_type(const String& type) {
    element_type = type;
}

String ClayDebugElement::get_element_type() const {
    return element_type;
}

void ClayDebugElement::set_z_index(int z) {
    z_index = z;
}

int ClayDebugElement::get_z_index() const {
    return z_index;
}

String ClayDebugElement::get_debug_info() const {
    return String("ID: ") + element_id +
           String(" | Type: ") + element_type +
           String(" | Bounds: (") + String::num(bounding_box.position.x, 0) +
           String(", ") + String::num(bounding_box.position.y, 0) +
           String(") ") + String::num(bounding_box.size.x, 0) +
           String("x") + String::num(bounding_box.size.y, 0);
}
