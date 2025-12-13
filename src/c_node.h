#ifndef C_NODE_H
#define C_NODE_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>

// Include the C header
#include "c_node_internal.h"

namespace godot {

/**
 * C_Node - A Godot Node with pure C internal logic
 *
 * This demonstrates how to wrap C code in a GDExtension node.
 * The class interface uses godot-cpp (required for GDExtension),
 * but all internal data and logic is implemented in pure C.
 *
 * This pattern is useful for:
 * - Integrating existing C libraries
 * - Performance-critical code that benefits from C
 * - Code that needs to be shared with non-C++ projects
 */
class C_Node : public Node {
    GDCLASS(C_Node, Node);

private:
    // Pure C data structure
    C_NodeData c_data;

protected:
    static void _bind_methods();

public:
    C_Node();
    ~C_Node();

    // Lifecycle methods
    void _ready() override;
    void _process(double delta) override;

    // Properties (wrapping C functions)
    void set_message(const String &message);
    String get_message() const;

    void set_speed(float speed);
    float get_speed() const;

    void set_active(bool active);
    bool is_active() const;

    // Methods
    int get_counter() const;
    void reset_counter();

    // Utility
    String get_info() const;
};

} // namespace godot

#endif // C_NODE_H
