#ifndef POLARIS_PROPERTY_MACROS_H
#define POLARIS_PROPERTY_MACROS_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/object.hpp>

// =============================================================================
// Polaris Property Macros
// =============================================================================
//
// PURPOSE: Define properties once, generate accessors and bindings automatically.
//
// USAGE:
//   1. Define X-macro with property metadata:
//      #define MY_PROPS(X, ...) \
//          X(field_name, cpp_type, GODOT_TYPE, "inspector/path", "hint", __VA_ARGS__)
//
//   2. In class declaration, generate accessors:
//      POLARIS_ACCESSORS(ComponentType, m_member, MY_PROPS)
//
//   3. In _bind_methods(), generate bindings:
//      POLARIS_BIND(ClassName, MY_PROPS)
//
// PATTERN:
//   - Structs are plain POD (no macros in struct definition)
//   - Config (serialized) and State (runtime) are separate structs
//   - Accessors just passthrough values + call sync<T>()
//   - Systems do ALL unit conversions when reading values
//
// EXAMPLE:
//   // Config struct - plain POD
//   struct PlayerLook {
//       float sensitivity_pct{};
//       bool invert_y{};
//   };
//
//   // State struct - runtime only
//   struct PlayerLookState {
//       float yaw{};
//       float pitch{};
//   };
//
//   // X-macro - define once
//   #define PLAYER_LOOK_PROPS(X, ...) \
//       X(sensitivity_pct, float, FLOAT, "look/sensitivity", "", __VA_ARGS__) \
//       X(invert_y,        bool,  BOOL,  "look/invert_y",    "", __VA_ARGS__)
//
//   // In class - generates accessors
//   class PlayerPawn : public Pawn {
//       PlayerLook m_look;
//   public:
//       POLARIS_ACCESSORS(PlayerLook, m_look, PLAYER_LOOK_PROPS)
//   };
//
//   // In _bind_methods - generates bindings
//   void PlayerPawn::_bind_methods() {
//       ADD_GROUP("Look", "look/");
//       POLARIS_BIND(PlayerPawn, PLAYER_LOOK_PROPS)
//   }
//
//   // In system - conversions happen HERE
//   float sensitivity = Units::Sensitivity::to_godot(cfg.sensitivity_pct);
//
// =============================================================================

// -----------------------------------------------------------------------------
// POLARIS_ACCESSORS - Generate get/set methods
// -----------------------------------------------------------------------------
// Expands X-macro to create accessor pairs that passthrough + sync.
//
// Generated code per property:
//   void set_name(type v) { member.name = v; sync<Comp>(member); }
//   type get_name() const { return member.name; }

#define POLARIS_ACCESSOR_(name, type, vtype, path, hint, Comp, member) \
    void set_##name(type v) { member.name = v; sync<Comp>(member); } \
    type get_##name() const { return member.name; }

#define POLARIS_ACCESSORS(Comp, member, PROPS) \
    PROPS(POLARIS_ACCESSOR_, Comp, member)

// -----------------------------------------------------------------------------
// POLARIS_BIND - Generate ClassDB bindings
// -----------------------------------------------------------------------------
// Expands X-macro to create ClassDB method bindings and properties.
//
// Generated code per property:
//   ClassDB::bind_method(D_METHOD("set_name", "v"), &Class::set_name);
//   ClassDB::bind_method(D_METHOD("get_name"), &Class::get_name);
//   ADD_PROPERTY(PropertyInfo(...), "set_name", "get_name");

#define POLARIS_BIND_(name, type, vtype, path, hint, Class) \
    godot::ClassDB::bind_method(godot::D_METHOD("set_" #name, "v"), &Class::set_##name); \
    godot::ClassDB::bind_method(godot::D_METHOD("get_" #name), &Class::get_##name); \
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::vtype, path, \
        hint[0] ? godot::PROPERTY_HINT_ENUM : godot::PROPERTY_HINT_NONE, hint), \
        "set_" #name, "get_" #name);

#define POLARIS_BIND(Class, PROPS) \
    PROPS(POLARIS_BIND_, Class)

#endif // POLARIS_PROPERTY_MACROS_H
