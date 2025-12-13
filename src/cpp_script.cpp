#include "cpp_script.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void CPPScript::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_time_elapsed"), &CPPScript::get_time_elapsed);
    ClassDB::bind_method(D_METHOD("find_child_by_name", "name"), &CPPScript::find_child_by_name);
}

CPPScript::CPPScript() {
}

CPPScript::~CPPScript() {
}

void CPPScript::_ready() {
    set_process(true);
    set_physics_process(true);
    _is_initialized = true;
    on_start();
}

void CPPScript::_process(double delta) {
    _time_elapsed += delta;
    on_update(delta);
}

void CPPScript::_physics_process(double delta) {
    on_fixed_update(delta);
}

void CPPScript::_exit_tree() {
    on_destroy();
}

double CPPScript::get_time_elapsed() const {
    return _time_elapsed;
}

Node* CPPScript::find_child_by_name(const String &name) const {
    return find_child(name, true, false);
}
