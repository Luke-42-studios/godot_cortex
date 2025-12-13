#ifndef CPP_SCRIPT_H
#define CPP_SCRIPT_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {

/**
 * CPPScript - Base class for C++ gameplay scripts
 *
 * Inherit from this class to create gameplay logic in C++.
 * Works like a GDScript but in C++.
 *
 * Example usage:
 *   class PlayerController : public CPPScript { ... }
 *   class EnemyAI : public CPPScript { ... }
 */
class CPPScript : public Node {
    GDCLASS(CPPScript, Node);

private:
    bool _is_initialized = false;
    double _time_elapsed = 0.0;

protected:
    static void _bind_methods();

    // Override these in your derived classes
    virtual void on_start() {}
    virtual void on_update(double delta) {}
    virtual void on_fixed_update(double delta) {}
    virtual void on_destroy() {}

public:
    CPPScript();
    ~CPPScript();

    // Godot lifecycle
    void _ready() override;
    void _process(double delta) override;
    void _physics_process(double delta) override;
    void _exit_tree() override;

    // Utility methods available to all scripts
    double get_time_elapsed() const;

    // Quick access to common nodes
    Node* find_child_by_name(const String &name) const;

    // Helper to get typed nodes
    template<typename T>
    T* get_node_as(const String &path) const {
        return Object::cast_to<T>(get_node_or_null(NodePath(path)));
    }
};

} // namespace godot

#endif // CPP_SCRIPT_H
