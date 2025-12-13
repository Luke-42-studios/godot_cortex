#ifndef GDFRAMEWORK_REGISTER_TYPES_H
#define GDFRAMEWORK_REGISTER_TYPES_H

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void initialize_gdframework_module(ModuleInitializationLevel p_level);
void uninitialize_gdframework_module(ModuleInitializationLevel p_level);

#endif // GDFRAMEWORK_REGISTER_TYPES_H
