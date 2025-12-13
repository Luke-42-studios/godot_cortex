#ifndef CPP_NODE_H
#define CPP_NODE_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

/**
 * CPP_Node - A custom Node implemented in C++ via GDExtension
 *
 * This is a simple example node that demonstrates how to create
 * custom nodes using godot-cpp. It extends Node and adds some
 * basic properties and methods.
 */
class CPP_Node : public Node {
    GDCLASS(CPP_Node, Node);

private:
    String custom_message;
    int update_count = 0;
    bool processing_enabled = false;

protected:
    static void _bind_methods();

public:
    CPP_Node();
    ~CPP_Node();

    // Lifecycle methods
    void _ready() override;
    void _process(double delta) override;

    // Custom methods
    void set_custom_message(const String &message);
    String get_custom_message() const;

    void set_processing_enabled(bool enabled);
    bool get_processing_enabled() const;

    int get_update_count() const;
    void reset_update_count();

    // Utility method
    String get_info() const;
};

} // namespace godot

#endif // CPP_NODE_H
