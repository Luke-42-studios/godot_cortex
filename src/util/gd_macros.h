#ifndef GD_MACROS_H
#define GD_MACROS_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

/**
 * GD_MACROS - Simplified binding macros for GDExtension
 *
 * These macros reduce boilerplate when creating Godot nodes in C++.
 * Instead of manually writing _bind_methods(), you can use these
 * macros to auto-generate bindings.
 *
 * Usage Example:
 * ==============
 *
 * // In header file:
 * class MyNode : public Node {
 *     GDCLASS(MyNode, Node);
 *     GD_BIND_BEGIN(MyNode)
 *         GD_PROP(health, INT)
 *         GD_PROP(name, STRING)
 *         GD_PROP_RANGE(speed, FLOAT, "0,100,0.1")
 *         GD_METHOD(take_damage)
 *         GD_SIGNAL(died)
 *     GD_BIND_END()
 * public:
 *     int health = 100;
 *     String name = "Player";
 *     float speed = 10.0f;
 *
 *     void take_damage(int amount) { health -= amount; }
 * };
 */

namespace godot {

// ============================================================================
// Property Type Helpers
// ============================================================================

// Maps C++ types to Variant::Type
template<typename T> struct VariantType { static constexpr Variant::Type value = Variant::NIL; };
template<> struct VariantType<bool> { static constexpr Variant::Type value = Variant::BOOL; };
template<> struct VariantType<int> { static constexpr Variant::Type value = Variant::INT; };
template<> struct VariantType<int64_t> { static constexpr Variant::Type value = Variant::INT; };
template<> struct VariantType<float> { static constexpr Variant::Type value = Variant::FLOAT; };
template<> struct VariantType<double> { static constexpr Variant::Type value = Variant::FLOAT; };
template<> struct VariantType<String> { static constexpr Variant::Type value = Variant::STRING; };
template<> struct VariantType<Vector2> { static constexpr Variant::Type value = Variant::VECTOR2; };
template<> struct VariantType<Vector3> { static constexpr Variant::Type value = Variant::VECTOR3; };
template<> struct VariantType<Color> { static constexpr Variant::Type value = Variant::COLOR; };
template<> struct VariantType<Rect2> { static constexpr Variant::Type value = Variant::RECT2; };

} // namespace godot

// ============================================================================
// Simple Property Binding (generates getter/setter automatically)
// ============================================================================

// Declares a property with auto-generated getter/setter
// Use in class body, then define the member variable
#define GD_PROPERTY(type, name, default_val) \
    private: type _##name = default_val; \
    public: \
    void set_##name(type value) { _##name = value; } \
    type get_##name() const { return _##name; }

// Binds a property declared with GD_PROPERTY
#define GD_BIND_PROPERTY(cls, type, name) \
    ClassDB::bind_method(D_METHOD("set_" #name, "value"), &cls::set_##name); \
    ClassDB::bind_method(D_METHOD("get_" #name), &cls::get_##name); \
    ADD_PROPERTY(PropertyInfo(VariantType<type>::value, #name), "set_" #name, "get_" #name);

// Binds a property with a hint (e.g., range)
#define GD_BIND_PROPERTY_HINT(cls, type, name, hint, hint_string) \
    ClassDB::bind_method(D_METHOD("set_" #name, "value"), &cls::set_##name); \
    ClassDB::bind_method(D_METHOD("get_" #name), &cls::get_##name); \
    ADD_PROPERTY(PropertyInfo(VariantType<type>::value, #name, hint, hint_string), "set_" #name, "get_" #name);

// ============================================================================
// Method Binding Helpers
// ============================================================================

// Bind a method with no arguments
#define GD_BIND_METHOD(cls, method) \
    ClassDB::bind_method(D_METHOD(#method), &cls::method);

// Bind a method with 1 argument
#define GD_BIND_METHOD_1(cls, method, arg1) \
    ClassDB::bind_method(D_METHOD(#method, #arg1), &cls::method);

// Bind a method with 2 arguments
#define GD_BIND_METHOD_2(cls, method, arg1, arg2) \
    ClassDB::bind_method(D_METHOD(#method, #arg1, #arg2), &cls::method);

// Bind a method with 3 arguments
#define GD_BIND_METHOD_3(cls, method, arg1, arg2, arg3) \
    ClassDB::bind_method(D_METHOD(#method, #arg1, #arg2, #arg3), &cls::method);

// ============================================================================
// Signal Binding Helpers
// ============================================================================

// Bind a signal with no arguments
#define GD_BIND_SIGNAL(name) \
    ADD_SIGNAL(MethodInfo(#name));

// Bind a signal with 1 argument
#define GD_BIND_SIGNAL_1(name, type1, arg1) \
    ADD_SIGNAL(MethodInfo(#name, PropertyInfo(type1, #arg1)));

// Bind a signal with 2 arguments
#define GD_BIND_SIGNAL_2(name, type1, arg1, type2, arg2) \
    ADD_SIGNAL(MethodInfo(#name, PropertyInfo(type1, #arg1), PropertyInfo(type2, #arg2)));

// ============================================================================
// Quick Node Definition (Single Header)
// ============================================================================

// Start a simple node class definition
#define GD_NODE_BEGIN(cls, base) \
    class cls : public base { \
        GDCLASS(cls, base); \
    protected: \
        static void _bind_methods() {

// End the binding section and start the public interface
#define GD_NODE_BODY() \
        } \
    public: \
        cls() {} \
        ~cls() {}

// End the class
#define GD_NODE_END() \
    };

// ============================================================================
// All-in-one Property Macro (declare + bind in one place)
// ============================================================================

// For use INSIDE _bind_methods - binds existing getter/setter
#define BIND_PROP(cls, name, type) \
    ClassDB::bind_method(D_METHOD("set_" #name, "value"), &cls::set_##name); \
    ClassDB::bind_method(D_METHOD("get_" #name), &cls::get_##name); \
    ADD_PROPERTY(PropertyInfo(type, #name), "set_" #name, "get_" #name)

#define BIND_PROP_RANGE(cls, name, type, range) \
    ClassDB::bind_method(D_METHOD("set_" #name, "value"), &cls::set_##name); \
    ClassDB::bind_method(D_METHOD("get_" #name), &cls::get_##name); \
    ADD_PROPERTY(PropertyInfo(type, #name, PROPERTY_HINT_RANGE, range), "set_" #name, "get_" #name)

#define BIND_METHOD(cls, name) \
    ClassDB::bind_method(D_METHOD(#name), &cls::name)

#define BIND_METHOD_ARGS(cls, name, ...) \
    ClassDB::bind_method(D_METHOD(#name, __VA_ARGS__), &cls::name)

#endif // GD_MACROS_H
