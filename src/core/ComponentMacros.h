#ifndef POLARIS_COMPONENT_MACROS_H
#define POLARIS_COMPONENT_MACROS_H

#include "DebugRegistry.h"
#include <godot_cpp/core/class_db.hpp>

// =============================================================================
// Component Declaration Macros
// =============================================================================
//
// PURPOSE: Provide consistent patterns for declaring ECS components with
// automatic debug capability for states.
//
// PATTERN:
//   - CONFIG(Name) - Serialized config, inspector-bound via X-macros
//   - STATE(Name)  - Runtime state, gets .debug() method automatically
//
// =============================================================================

// =============================================================================
// STATE(Name) - Runtime state with debug capability
// =============================================================================
//
// Creates Name_Base struct and Name alias wrapping in Debuggable<>.
// All states automatically get .debug() method that reads from DebugRegistry.
//
// USAGE:
//   STATE(JumpVisualState) {
//       bool just_jumped{};
//       float air_time{};
//   };
//
//   // In system:
//   if (state.debug()) {
//       UtilityFunctions::print("air_time=", state.air_time);
//   }
//
// CREATES:
//   struct JumpVisualState_Base { ... };
//   using JumpVisualState = Polaris::Debuggable<JumpVisualState_Base>;

#define STATE(Name)          \
    struct Name##_Base;      \
    using Name = Polaris::Debuggable<Name##_Base>; \
    struct Name##_Base

// =============================================================================
// CONFIG(Name) - Serialized config (marker for consistency)
// =============================================================================
//
// Currently just a marker establishing the Config naming convention.
// May add capabilities later (clone, reset_defaults, validation).
//
// USAGE:
//   CONFIG(JumpVisualConfig) {
//       float anticipation{0.02f};
//       bool enabled{true};
//   };
//
// CREATES:
//   struct JumpVisualConfig { ... };

#define CONFIG(Name) struct Name

// =============================================================================
// Debug Accessor Macros (DEBUG_ENABLED only)
// =============================================================================
//
// These macros only exist in debug builds. Wrap usage in #ifdef DEBUG_ENABLED
// to avoid compile errors in release builds.
//
// USAGE:
//   // In class declaration:
//   #ifdef DEBUG_ENABLED
//   POLARIS_DEBUG_ACCESSOR(MyState, my_state)
//   #endif
//
//   // In _bind_methods:
//   #ifdef DEBUG_ENABLED
//   ADD_GROUP("Debug", "debug/");
//   POLARIS_DEBUG_BIND(MyClass, my_state, "debug/my_state")
//   #endif
//
// GENERATES:
//   void set_debug_my_state(bool v) - Sets flag in DebugRegistry
//   bool get_debug_my_state() const - Gets flag from DebugRegistry
//   Property at "debug/my_state" in Godot inspector

#ifdef DEBUG_ENABLED

#define POLARIS_DEBUG_ACCESSOR(StateType, name)                                       \
    void set_debug_##name(bool v) { Polaris::DebugRegistry::set<StateType##_Base>(v); } \
    bool get_debug_##name() const { return Polaris::DebugRegistry::get<StateType##_Base>(); }

#define POLARIS_DEBUG_BIND(Class, name, path)                                                        \
    godot::ClassDB::bind_method(godot::D_METHOD("set_debug_" #name, "v"), &Class::set_debug_##name); \
    godot::ClassDB::bind_method(godot::D_METHOD("get_debug_" #name), &Class::get_debug_##name);      \
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, path), "set_debug_" #name, "get_debug_" #name);

#endif // DEBUG_ENABLED

#endif // POLARIS_COMPONENT_MACROS_H
